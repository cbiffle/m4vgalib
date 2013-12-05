#include "vga/vga.h"

#include "lib/common/attribute_macros.h"
#include "lib/armv7m/exceptions.h"
#include "lib/armv7m/exception_table.h"
#include "lib/armv7m/scb.h"

#include "lib/stm32f4xx/adv_timer.h"
#include "lib/stm32f4xx/ahb.h"
#include "lib/stm32f4xx/apb.h"
#include "lib/stm32f4xx/dma.h"
#include "lib/stm32f4xx/flash.h"
#include "lib/stm32f4xx/gpio.h"
#include "lib/stm32f4xx/interrupts.h"
#include "lib/stm32f4xx/rcc.h"
#include "lib/stm32f4xx/syscfg.h"

#include "vga/arena.h"
#include "vga/copy_words.h"
#include "vga/timing.h"
#include "vga/measurement.h"

using stm32f4xx::AdvTimer;
using stm32f4xx::AhbPeripheral;
using stm32f4xx::ApbPeripheral;
using stm32f4xx::Dma;
using stm32f4xx::dma2;
using stm32f4xx::flash;
using stm32f4xx::Gpio;
using stm32f4xx::gpioc;
using stm32f4xx::gpioe;
using stm32f4xx::Interrupt;
using stm32f4xx::rcc;
using stm32f4xx::syscfg;
using stm32f4xx::tim8;
using stm32f4xx::Word;

#define IN_SCAN_RAM SECTION(".vga_scan_ram")
#define IN_LOCAL_RAM SECTION(".vga_local_ram")

#define RAM_CODE SECTION(".ramcode")

namespace vga {

/*******************************************************************************
 * Driver state and configuration.
 */

// Used to adjust size of scan_buffer, below.
static constexpr unsigned max_pixels_per_line = 800;

// The current mode, set by select_mode.
static Mode *current_mode;

// The hblank callback action.
static Callback hblank_callback;

// [0, current_mode.video_end_line).  Updated at front porch interrupt.
static unsigned volatile current_line;

/*
 * The vertical timing state.  This is a Gray code and the bits have meaning.
 * See the inspector functions below.
 */
enum class State {
  blank     = 0b00,
  starting  = 0b01,
  active    = 0b11,
  finishing = 0b10,
};

// Should we be producing a video signal?
inline bool is_displayed_state(State s) {
  return static_cast<unsigned>(s) & 0b10;
}

// Should we be rendering a scanline?
inline bool is_rendered_state(State s) {
  return static_cast<unsigned>(s) & 0b01;
}

// Finally, the actual variable.
static State volatile state;

// This is the DMA source for scan-out, filled during pend_sv.
// It's aligned for DMA.
// It contains four trailing pixels that are kept black for blanking.
ALIGNED(4) IN_SCAN_RAM
static unsigned char scan_buffer[max_pixels_per_line + 4];

// This is the intermediate buffer used during rasterization.
// It should be close to the CPU and need not be DMA-capable.
// It's aligned to make copying it more efficient.
ALIGNED(4) IN_LOCAL_RAM
static unsigned char working_buffer[max_pixels_per_line];


/*******************************************************************************
 * Driver API.
 */

void init() {
  // Turn on I/O compensation cell to reduce noise on power supply.
  rcc.enable_clock(ApbPeripheral::syscfg);
  syscfg.write_cmpcr(syscfg.read_cmpcr().with_cmp_pd(true));

  // Turn a bunch of stuff on.
  rcc.enable_clock(ApbPeripheral::tim8);
  rcc.enable_clock(AhbPeripheral::gpioc);  // Sync signals
  rcc.enable_clock(AhbPeripheral::gpioe);  // Video
  rcc.enable_clock(AhbPeripheral::dma2);

  // Hold TIM8 in reset until we do mode-setting.
  rcc.enter_reset(ApbPeripheral::tim8);

  set_irq_priority(Interrupt::tim8_cc, 0);
  set_exception_priority(armv7m::Exception::pend_sv, 0xFF);

  msigs_init();

  video_off();
}

void video_off() {
  // Tristate all output ports.
  gpioc.set_mode((1 << 6) | (1 << 7), Gpio::Mode::input);
  gpioc.set_pull((1 << 6) | (1 << 7), Gpio::Pull::none);

  gpioe.set_mode(0xFF00, Gpio::Mode::input);
  gpioe.set_pull(0xFF00, Gpio::Pull::none);
}

void video_on() {
  // Configure PC6 to produce hsync using TIM8_CH1
  gpioc.set_alternate_function(Gpio::p6, 3);
  gpioc.set_output_type(Gpio::p6, Gpio::OutputType::push_pull);
  gpioc.set_output_speed(Gpio::p6, Gpio::OutputSpeed::fast_50mhz);
  gpioc.set_mode(Gpio::p6, Gpio::Mode::alternate);

  // Configure PC7 as GPIO output.
  gpioc.set_output_type(Gpio::p7, Gpio::OutputType::push_pull);
  gpioc.set_output_speed(Gpio::p7, Gpio::OutputSpeed::fast_50mhz);
  gpioc.set_mode(Gpio::p7, Gpio::Mode::gpio);

  // Configure the high byte of port E for parallel video.
  // Using 100MHz output speed gets slightly sharper transitions than 50MHz.
  gpioe.set_output_type(0xFF00, Gpio::OutputType::push_pull);
  gpioe.set_output_speed(0xFF00, Gpio::OutputSpeed::high_100mhz);
  gpioe.set_mode(0xFF00, Gpio::Mode::gpio);
}

void select_mode(Mode *mode, Callback cb) {
  // Disable outputs during mode change.
  video_off();

  // Place TIM8 in reset, stopping all timing, and disable its interrupt.
  disable_irq(Interrupt::tim8_cc);
  rcc.enter_reset(ApbPeripheral::tim8);
  clear_pending_irq(Interrupt::tim8_cc);

  // Busy-wait for pending DMA to complete.
  while (dma2.stream1.read_cr().get_en());

  if (current_mode) current_mode->deactivate();

  arena_reset();

  mode->activate();
  Timing const &timing = mode->get_timing();

  // Switch to new CPU clock settings.
  rcc.configure_clocks(*timing.clock_config);

  // Enable Flash cache and prefetching to try and reduce jitter.
  flash.write_acr(flash.read_acr()
                  .with_dcen(true)
                  .with_icen(true)
                  .with_prften(true));

  // Configure TIM8 for horizontal sync generation.
  rcc.leave_reset(ApbPeripheral::tim8);
  tim8.write_psc(4 - 1);  // Count in pixels, 1 pixel = 4 CCLK

  tim8.write_arr(timing.line_pixels - 1);
  tim8.write_ccr1(timing.sync_pixels);
  tim8.write_ccr2(timing.sync_pixels
                  + timing.back_porch_pixels - timing.video_lead);
  tim8.write_ccr3(timing.sync_pixels
                  + timing.back_porch_pixels + timing.video_pixels);

  tim8.write_ccmr1(AdvTimer::ccmr1_value_t()
                   .with_oc1m(AdvTimer::OcMode::pwm1)
                   .with_cc1s(AdvTimer::ccmr1_value_t::cc1s_t::output));

  tim8.write_bdtr(AdvTimer::bdtr_value_t()
                  .with_ossr(true)
                  .with_moe(true));

  tim8.write_ccer(AdvTimer::ccer_value_t()
                  .with_cc1e(true)
                  .with_cc1p(
                      timing.hsync_polarity == Timing::Polarity::negative));

  tim8.write_dier(AdvTimer::dier_value_t()
                  .with_cc2ie(true)    // Interrupt at start of active video.
                  .with_cc3ie(true));  // Interrupt at end of active video.

  // Note: TIM8 is still not running.

  switch (timing.vsync_polarity) {
    case Timing::Polarity::positive: gpioc.clear(1 << 7); break;
    case Timing::Polarity::negative: gpioc.set  (1 << 7); break;
  }

  // Scribble over working buffer to help catch bugs.
  for (unsigned i = 0; i < sizeof(working_buffer); i += 2) {
    working_buffer[i] = 0xFF;
    working_buffer[i + 1] = 0x00;
  }
  // Blank the final four pixels of the scan buffer.
  scan_buffer[timing.video_pixels + 0] = 0;
  scan_buffer[timing.video_pixels + 1] = 0;
  scan_buffer[timing.video_pixels + 2] = 0;
  scan_buffer[timing.video_pixels + 3] = 0;

  // Set up global state.
  current_mode = mode;
  current_line = 0;
  hblank_callback = cb;

  // Start the timer.
  enable_irq(Interrupt::tim8_cc);
  tim8.write_cr1(tim8.read_cr1().with_cen(true));

  // Enable display and sync signals.
  video_on();
}

void wait_for_vblank() {
  while (!in_vblank());
}

bool in_vblank() {
  return current_line < current_mode->get_timing().video_start_line;
}

void sync_to_vblank() {
  while (in_vblank());
  wait_for_vblank();
}

}  // namespace vga

/*******************************************************************************
 * Horizontal timing interrupt.
 */
RAM_CODE void stm32f4xx_tim8_cc_handler() {
  // We have to clear our interrupt flags, or this will recur.
  auto sr = tim8.read_sr();
  tim8.write_sr(sr.with_cc2if(false).with_cc3if(false));

  vga::Mode &mode = *vga::current_mode;
  vga::Timing const &timing = mode.get_timing();

  if (sr.get_cc2if()) {
    // CC2 indicates start of active video (end of back porch).
    // This only matters in displayed states.
    if (is_displayed_state(vga::state)) {
      // Clear stream 1 flags (lifcr is a write-1-to-clear register).
      dma2.write_lifcr(Dma::lifcr_value_t()
                       .with_cdmeif1(true)
                       .with_cteif1(true)
                       .with_chtif1(true)
                       .with_ctcif1(true));

      auto &st = dma2.stream1;

      // Prepare to transfer pixels as words, plus the final black word.
      st.write_ndtr(timing.video_pixels / 4 + 1);

      // Set addresses.  Note that we're using memory as the peripheral side.
      // This DMA controller is a little odd.
      st.write_par(reinterpret_cast<Word>(&vga::scan_buffer));
      st.write_m0ar(0x40021015);  // High byte of GPIOE ODR (hack hack)

      // Configure FIFO.
      st.write_fcr(Dma::Stream::fcr_value_t()
                   .with_fth(Dma::Stream::fcr_value_t::fth_t::quarter)
                   .with_dmdis(true)
                   .with_feie(false));

      /*
       * Configure and enable the DMA stream.  The configuration used here
       * deserves more discussion.
       *
       * As noted above, our "peripheral" is RAM and our "memory" is the GPIO
       * unit.  In memory-to-memory mode (which we use) the distinction is
       * not useful, since the peripherals are memory-mapped; the controller
       * insists that "peripheral" be source and "memory" be destination in that
       * mode.  The key here is that the transfer runs at full speed.  On the
       * STM32F407 the transfer will not exceed one unit per 4 AHB cycles.  The
       * reason for this is not obvious.
       *
       * Address incrementation on this chip is independent from whether an
       * address is considered "peripheral" or "memory."  Here we auto-increment
       * the peripheral address (to walk through the scan buffer) while leaving
       * the memory address fixed (at the appropriate byte of the GPIO port).
       *
       * Because we're using memory-to-memory, the hardware enforces several
       * restrictions:
       *
       *  1. We're stuck using DMA2.  DMA1 can't do it.
       *  2. We're required to use the FIFO -- direct mode is verboten.
       *  3. We can't use circular mode.  (Double-buffer mode appears permitted,
       *     but I haven't tried it.)
       *
       * Fortunately we can tie the FIFO to a tree by giving it a really low
       * threshold level.
       *
       * I have not experimented with burst modes, but I suspect they'll make
       * the timing less regular.
       *
       * Note that the priority (pl field) is only used for arbitration between
       * streams of the same DMA controller.  The STM32F4 does not provide any
       * control over the AHB matrix arbitration scheme, unlike (say) the NXP
       * LPC1788.  Shame, that.  It means we have to be very careful about our
       * use of the bus matrix during scan-out.
       */
      typedef Dma::Stream::cr_value_t cr_t;
      st.write_cr(Dma::Stream::cr_value_t()
                  // Originally chosen to play nice with TIM8.  Now, arbitrary.
                  .with_chsel(7)
                  .with_pl(cr_t::pl_t::very_high)
                  .with_dir(cr_t::dir_t::memory_to_memory)
                  // Input settings:
                  .with_pburst(Dma::Stream::BurstSize::single)
                  .with_psize(Dma::Stream::TransferSize::word)
                  .with_pinc(true)
                  // Output settings:
                  .with_mburst(Dma::Stream::BurstSize::single)
                  .with_msize(Dma::Stream::TransferSize::byte)
                  .with_minc(false)
                  // Look at all these options we don't use:
                  .with_dbm(false)
                  .with_pincos(false)
                  .with_circ(false)
                  .with_pfctrl(false)
                  .with_tcie(false)
                  .with_htie(false)
                  .with_teie(false)
                  .with_dmeie(false)
                  // Finally, enable.
                  .with_en(true));
    }
  }

  if (sr.get_cc3if()) {
    // CC3 indicates end of active video (start of front porch).
    unsigned line = vga::current_line;

    if (line == 0) {
      // Start of frame!  Time to stop displaying pixels.
      vga::state = vga::State::blank;
      // TODO(cbiffle): latch configuration changes.
    } else if (line == timing.vsync_start_line
            || line == timing.vsync_end_line) {
      // Either edge of vsync pulse.
      gpioc.toggle(Gpio::p7);
    } else if (line ==
                    static_cast<unsigned short>(timing.video_start_line - 1)) {
      // Time to start generating the first scan buffer.
      vga::state = vga::State::starting;
    } else if (line == timing.video_start_line) {
      // Time to start output.
      vga::state = vga::State::active;
    } else if (line == static_cast<unsigned short>(timing.video_end_line - 1)) {
      // Time to stop rendering new scan buffers.
      vga::state = vga::State::finishing;
      line = static_cast<unsigned>(-1);  // Make line roll over to zero.
    }

    vga::current_line = line + 1;

    // Pend a PendSV to process hblank tasks.
    armv7m::scb.write_icsr(armv7m::Scb::icsr_value_t().with_pendsvset(true));
  }
}

RAM_CODE
void v7m_pend_sv_handler() {
  vga::Callback c = vga::hblank_callback;
  if (c) c();

  if (is_rendered_state(vga::state)) {
    vga::Timing const &timing = vga::current_mode->get_timing();
    // Flip working_buffer into scan_buffer.
    // We know its contents are ready because otherwise we wouldn't take a new
    // PendSV.
    // Note that GCC can't see that we've aligned the buffers correctly, so we
    // have to do a multi-cast dance. :-/
    copy_words(reinterpret_cast<Word const *>(
                   static_cast<void *>(vga::working_buffer)),
               reinterpret_cast<Word *>(
                   static_cast<void *>(vga::scan_buffer)),
               timing.video_pixels / 4);

    vga::current_mode->rasterize(vga::current_line, vga::working_buffer);
  }
}
