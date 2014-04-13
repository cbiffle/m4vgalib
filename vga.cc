#include "vga/vga.h"

#include "etl/common/types.h"
#include "etl/common/size.h"

#include "etl/common/attribute_macros.h"
#include "etl/armv7m/exceptions.h"
#include "etl/armv7m/exception_table.h"
#include "etl/armv7m/instructions.h"
#include "etl/armv7m/scb.h"
#include "etl/armv7m/types.h"

#include "etl/stm32f4xx/rcc.h"

#include "etl/stm32f4xx/adv_timer.h"
#include "etl/stm32f4xx/ahb.h"
#include "etl/stm32f4xx/apb.h"
#include "etl/stm32f4xx/dbg.h"
#include "etl/stm32f4xx/dma.h"
#include "etl/stm32f4xx/flash.h"
#include "etl/stm32f4xx/gpio.h"
#include "etl/stm32f4xx/interrupt_table.h"
#include "etl/stm32f4xx/interrupts.h"
#include "etl/stm32f4xx/syscfg.h"

#include "vga/arena.h"
#include "vga/copy_words.h"
#include "vga/timing.h"
#include "vga/rasterizer.h"

using etl::common::Size;
using etl::common::UInt8;

using etl::armv7m::Scb;
using etl::armv7m::scb;
using etl::armv7m::Word;

using etl::stm32f4xx::AdvTimer;
using etl::stm32f4xx::AhbPeripheral;
using etl::stm32f4xx::ApbPeripheral;
using etl::stm32f4xx::Dbg;
using etl::stm32f4xx::dbg;
using etl::stm32f4xx::Dma;
using etl::stm32f4xx::dma2;
using etl::stm32f4xx::flash;
using etl::stm32f4xx::Gpio;
using etl::stm32f4xx::gpioc;
using etl::stm32f4xx::gpioe;
using etl::stm32f4xx::Interrupt;
using etl::stm32f4xx::rcc;
using etl::stm32f4xx::syscfg;
using etl::stm32f4xx::tim1;
using etl::stm32f4xx::tim8;

#define IN_SCAN_RAM ETL_SECTION(".vga_scan_ram")
#define IN_LOCAL_RAM ETL_SECTION(".vga_local_ram")

#define RAM_CODE ETL_SECTION(".ramcode")

namespace vga {

/*******************************************************************************
 * Driver state and configuration.
 */

// Used to adjust size of scan_buffer.
static constexpr unsigned max_pixels_per_line = 800;
// Used to size rasterization plan.
static constexpr unsigned max_lines = 600;

// A copy of the current Timing, held in RAM for fast access.
static Timing current_timing;

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
ETL_ALIGNED(4) IN_SCAN_RAM
static Pixel scan_buffer[max_pixels_per_line + 4];

// This is the intermediate buffer used during rasterization.
// It should be close to the CPU and need not be DMA-capable.
// It's aligned to make copying it more efficient.
ETL_ALIGNED(4) IN_LOCAL_RAM
static Pixel working_buffer[max_pixels_per_line];

static Rasterizer::LineShape working_buffer_shape;

static Band const *band_list_head;
static Band current_band;


/*******************************************************************************
 * Driver API.
 */

void init() {
  // Turn on I/O compensation cell to reduce noise on power supply.
  rcc.enable_clock(ApbPeripheral::syscfg);
  syscfg.write_cmpcr(syscfg.read_cmpcr().with_cmp_pd(true));

  // Turn a bunch of stuff on.
  rcc.enable_clock(AhbPeripheral::gpioc);  // Sync signals
  rcc.enable_clock(AhbPeripheral::gpioe);  // Video
  rcc.enable_clock(AhbPeripheral::dma2);

  // Configure our interrupt priorities.  The scheme is:
  //  TIM8 (horizontal) gets highest priority.
  //  TIM1 (shock absorber) is set just lower.
  //  PendSV (rendering, user code) is lowest.
  // We could fit other stuff into the gaps later.
  // Note that PendSV is set using ARMv7-M priorities (0-255) and the others are
  // set using narrower SoC priorities (0-15).  This is a bit ugly.
  set_irq_priority(Interrupt::tim8_cc, 0);
  set_irq_priority(Interrupt::tim1_cc, 1);
  scb.set_exception_priority(etl::armv7m::Exception::pend_sv, 0xFF);

  // Enable Flash cache and prefetching to try and reduce jitter.
  // This only affects best-effort-level code, not anything realtime.
  flash.write_acr(flash.read_acr()
                  .with_dcen(true)
                  .with_icen(true)
                  .with_prften(true));

  band_list_head = 0;

  sync_off();
  video_off();
  arena_reset();
}

void sync_off() {
  gpioc.set_mode((1 << 6) | (1 << 7), Gpio::Mode::input);
  gpioc.set_pull((1 << 6) | (1 << 7), Gpio::Pull::down);
}

void video_off() {
  gpioe.set_mode(0xFF00, Gpio::Mode::input);
  gpioe.set_pull(0xFF00, Gpio::Pull::down);
}

void sync_on() {
  // Configure PC6 to produce hsync using TIM8_CH1
  gpioc.set_alternate_function(Gpio::p6, 3);
  gpioc.set_output_type(Gpio::p6, Gpio::OutputType::push_pull);
  gpioc.set_output_speed(Gpio::p6, Gpio::OutputSpeed::fast_50mhz);
  gpioc.set_mode(Gpio::p6, Gpio::Mode::alternate);

  // Configure PC7 as GPIO output.
  gpioc.set_output_type(Gpio::p7, Gpio::OutputType::push_pull);
  gpioc.set_output_speed(Gpio::p7, Gpio::OutputSpeed::fast_50mhz);
  gpioc.set_mode(Gpio::p7, Gpio::Mode::gpio);
}

void video_on() {
  // Configure the high byte of port E for parallel video.
  // Using 100MHz output speed gets slightly sharper transitions than 50MHz.
  gpioe.set_output_type(0xFF00, Gpio::OutputType::push_pull);
  gpioe.set_output_speed(0xFF00, Gpio::OutputSpeed::high_100mhz);
  gpioe.set_mode(0xFF00, Gpio::Mode::gpio);
}

/*
 * Sets up one of the two horizontal timers, which share almost all of their
 * init code.
 */
static void configure_h_timer(Timing const &timing,
                              ApbPeripheral p,
                              AdvTimer &tim) {
  rcc.enable_clock(p);
  rcc.leave_reset(p);
  tim.write_psc(4 - 1);  // Count in pixels, 1 pixel = 4 CCLK

  tim.write_arr(timing.line_pixels - 1);
  tim.write_ccr1(timing.sync_pixels);
  tim.write_ccr2(timing.sync_pixels
                 + timing.back_porch_pixels - timing.video_lead);
  tim.write_ccr3(timing.sync_pixels
                 + timing.back_porch_pixels + timing.video_pixels);

  tim.write_ccmr1(AdvTimer::ccmr1_value_t()
                  .with_oc1m(AdvTimer::OcMode::pwm1)
                  .with_cc1s(AdvTimer::ccmr1_value_t::cc1s_t::output));

  tim.write_bdtr(AdvTimer::bdtr_value_t()
                 .with_ossr(true)
                 .with_moe(true));

  tim.write_ccer(AdvTimer::ccer_value_t()
                 .with_cc1e(true)
                 .with_cc1p(
                     timing.hsync_polarity == Timing::Polarity::negative));

}

/*
 * Safely shut down a timer, so that we can reconfigure without interlocks.
 */
static void disable_h_timer(ApbPeripheral p,
                            Interrupt irq) {
  // Ensure that we'll receive no further interrupts.
  disable_irq(irq);
  // Ensure that the peripheral will generate no further interrupts.
  rcc.enter_reset(p);
  // In case of race condition between the above actions, clear any pending.
  clear_pending_irq(irq);
}

void configure_timing(Timing const &timing) {
  // Disable outputs during mode change.
  sync_off();
  video_off();

  // Place the horizontal timers in reset, disabling interrupts.
  disable_h_timer(ApbPeripheral::tim8, Interrupt::tim8_cc);
  disable_h_timer(ApbPeripheral::tim1, Interrupt::tim1_cc);

  // Busy-wait for pending DMA to complete.
  while (dma2.stream1.read_cr().get_en());

  // Switch to new CPU clock settings.
  rcc.configure_clocks(timing.clock_config);

  // Configure TIM1/8 for horizontal sync generation.
  configure_h_timer(timing, ApbPeripheral::tim1, tim1);
  configure_h_timer(timing, ApbPeripheral::tim8, tim8);

  // Adjust tim1's CC2 value back in time.
  tim1.write_ccr2(static_cast<Word>(tim1.read_ccr2()) - 7);

  // Configure tim1 to distribute its enable signal as its trigger output.
  tim1.write_cr2(AdvTimer::cr2_value_t()
                 .with_mms(AdvTimer::cr2_value_t::mms_t::enable)
                 .with_ccds(false));

  // Configure tim8 to trigger from tim1 and run forever.
  tim8.write_smcr(AdvTimer::smcr_value_t()
                  .with_ts(AdvTimer::smcr_value_t::ts_t::itr0)
                  .with_sms(AdvTimer::smcr_value_t::sms_t::trigger));

  // Turn on tim8's interrupts.
  tim8.write_dier(AdvTimer::dier_value_t()
                  .with_cc2ie(true)    // Interrupt at start of active video.
                  .with_cc3ie(true));  // Interrupt at end of active video.

  // Turn on only one of tim1's
  tim1.write_dier(AdvTimer::dier_value_t()
                  .with_cc2ie(true));  // Interrupt at start of active video.

  // Note: timers still not running.

  switch (timing.vsync_polarity) {
    case Timing::Polarity::positive: gpioc.clear(1 << 7); break;
    case Timing::Polarity::negative: gpioc.set  (1 << 7); break;
  }

  // Scribble over working buffer to help catch bugs.
  for (Size i = 0; i < sizeof(working_buffer); i += 2) {
    working_buffer[i] = 0xFF;
    working_buffer[i + 1] = 0x00;
  }

  // Blank the final four pixels of the scan buffer.
  scan_buffer[timing.video_pixels + 0] = 0;
  scan_buffer[timing.video_pixels + 1] = 0;
  scan_buffer[timing.video_pixels + 2] = 0;
  scan_buffer[timing.video_pixels + 3] = 0;

  // Set up global state.
  current_line = 0;
  current_timing = timing;

  // Halt both timers on debug.
  dbg.write_dbgmcu_apb2_fz(dbg.read_dbgmcu_apb2_fz()
                           .with_dbg_tim8_stop(true)
                           .with_dbg_tim1_stop(true));

  // Start TIM1, which starts TIM8.
  enable_irq(Interrupt::tim1_cc);
  enable_irq(Interrupt::tim8_cc);
  tim1.write_cr1(tim1.read_cr1().with_cen(true));

  sync_on();
}

void configure_band_list(Band const *head) {
  band_list_head = head;
}

void wait_for_vblank() {
  while (!in_vblank()) etl::armv7m::wait_for_interrupt();
}

bool in_vblank() {
  return current_line < current_timing.video_start_line;
}

void sync_to_vblank() {
  while (in_vblank()) etl::armv7m::wait_for_interrupt();
  wait_for_vblank();
}

void default_hblank_interrupt();  // decl hack
RAM_CODE void default_hblank_interrupt() {}

void hblank_interrupt()
  __attribute__((weak, alias("_ZN3vga24default_hblank_interruptEv")));

}  // namespace vga

/*******************************************************************************
 * Horizontal timing interrupt.
 */

RAM_CODE
__attribute__((noinline))
static void start_of_active_video() {
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
    st.write_ndtr(vga::working_buffer_shape.length / 4 + 1);

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

RAM_CODE
static void end_of_active_video() {
  vga::Timing const &timing = vga::current_timing;

  // CC3 indicates end of active video (start of front porch).
  unsigned line = vga::current_line;

  if (line == 0) {
    // Start of frame!  Time to stop displaying pixels.
    vga::state = vga::State::blank;
  } else if (line == timing.vsync_start_line
          || line == timing.vsync_end_line) {
    // Either edge of vsync pulse.
    gpioc.toggle(Gpio::p7);
  } else if (line ==
                  static_cast<unsigned short>(timing.video_start_line - 1)) {
    // Time to start generating the first scan buffer.
    vga::state = vga::State::starting;
    if (vga::band_list_head) {
      vga::current_band = *vga::band_list_head;
    } else {
      vga::current_band = { nullptr, 0, nullptr };
    }
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
  scb.write_icsr(Scb::icsr_value_t().with_pendsvset(true));
}

RAM_CODE void etl_stm32f4xx_tim1_cc_handler() {
  // We access this APB2 timer through the bridge on AHB1.  This implies
  // both wait states and resource conflicts with scanout.  Get done fast.
  tim1.write_sr(tim1.read_sr().with_cc2if(false));

  // Idle the processor until preempted by any higher-priority interrupt.
  // This ensures that the M4's D-code bus is available for exception entry.
  // NOTE: this behaves correctly on the M4, but WFI is not guaranteed to
  // actually do anything.
  etl::armv7m::wait_for_interrupt();
}

RAM_CODE void etl_stm32f4xx_tim8_cc_handler() {
  // We have to clear our interrupt flags, or this will recur.
  auto sr = tim8.read_sr();

  if (sr.get_cc2if()) {
    tim8.write_sr(sr.with_cc2if(false));
    start_of_active_video();
    return;
  }

  if (sr.get_cc3if()) {
    tim8.write_sr(sr.with_cc3if(false));
    end_of_active_video();
    return;
  }
}

RAM_CODE
static vga::Rasterizer *get_next_rasterizer() {
  if (vga::current_band.line_count) {
    --vga::current_band.line_count;
    return vga::current_band.rasterizer;
  } else {
    if (vga::current_band.next) {
      vga::current_band = *vga::current_band.next;
      return get_next_rasterizer();
    } else {
      return nullptr;
    }
  }
}

RAM_CODE
void etl_armv7m_pend_sv_handler() {
  vga::hblank_interrupt();

  if (is_rendered_state(vga::state)) {
    vga::Timing const &timing = vga::current_timing;
    // Flip working_buffer into scan_buffer.
    // We know its contents are ready because otherwise we wouldn't take a new
    // PendSV.
    // Note that GCC can't see that we've aligned the buffers correctly, so we
    // have to do a multi-cast dance. :-/
    ((Word *) (void *) vga::scan_buffer)[
        vga::working_buffer_shape.length / 4] = 0;
    copy_words(reinterpret_cast<Word const *>(
                   static_cast<void *>(vga::working_buffer)),
               reinterpret_cast<Word *>(
                   static_cast<void *>(vga::scan_buffer)),
               vga::working_buffer_shape.length / 4);

    unsigned line = vga::current_line;
    if (line >= timing.video_start_line && line <= timing.video_end_line) {
      unsigned visible_line = line - timing.video_start_line;
      vga::Rasterizer *r = get_next_rasterizer();
      if (r) {
        vga::Rasterizer::LineShape shape = r->rasterize(visible_line,
                                                        vga::working_buffer);
        vga::working_buffer_shape = shape;
        tim8.write_ccr2(timing.sync_pixels
                        + timing.back_porch_pixels - timing.video_lead
                        + shape.offset);
      }
    }
  }
}
