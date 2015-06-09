#include "vga/vga.h"

#include <atomic>
#include <cstddef>
#include <cstdint>

#include "etl/attribute_macros.h"
#include "etl/prediction.h"

#include "etl/armv7m/exceptions.h"
#include "etl/armv7m/exception_table.h"
#include "etl/armv7m/instructions.h"
#include "etl/armv7m/scb.h"
#include "etl/armv7m/types.h"

#include "etl/stm32f4xx/adv_timer.h"
#include "etl/stm32f4xx/ahb.h"
#include "etl/stm32f4xx/apb.h"
#include "etl/stm32f4xx/dbg.h"
#include "etl/stm32f4xx/dma.h"
#include "etl/stm32f4xx/flash.h"
#include "etl/stm32f4xx/gpio.h"
#include "etl/stm32f4xx/gp_timer.h"
#include "etl/stm32f4xx/interrupts.h"
#include "etl/stm32f4xx/interrupt_table.h"
#include "etl/stm32f4xx/rcc.h"
#include "etl/stm32f4xx/syscfg.h"

#include "vga/arena.h"
#include "vga/copy_words.h"
#include "vga/rasterizer.h"
#include "vga/timing.h"

using std::size_t;

using etl::armv7m::scb;
using etl::armv7m::Scb;
using etl::armv7m::Word;

using etl::stm32f4xx::AdvTimer;
using etl::stm32f4xx::AhbPeripheral;
using etl::stm32f4xx::ApbPeripheral;
using etl::stm32f4xx::dbg;
using etl::stm32f4xx::Dbg;
using etl::stm32f4xx::Dma;
using etl::stm32f4xx::dma2;
using etl::stm32f4xx::flash;
using etl::stm32f4xx::Gpio;
using etl::stm32f4xx::gpiob;
using etl::stm32f4xx::gpioe;
using etl::stm32f4xx::GpTimer;
using etl::stm32f4xx::Interrupt;
using etl::stm32f4xx::rcc;
using etl::stm32f4xx::syscfg;
using etl::stm32f4xx::tim1;
using etl::stm32f4xx::tim3;
using etl::stm32f4xx::tim4;

#define IN_SCAN_RAM ETL_SECTION(".vga_scan_ram")
#define IN_LOCAL_RAM ETL_SECTION(".vga_local_ram")

#define RAM_CODE ETL_SECTION(".ramcode")

namespace vga {

/*******************************************************************************
 * Driver state and configuration.
 */

// Used to adjust size of scan_buffer.
static constexpr unsigned max_pixels_per_line = 800;

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
alignas(4) IN_SCAN_RAM
static Pixel scan_buffer[max_pixels_per_line + 4];

// This is the intermediate buffer used during rasterization.
// It should be close to the CPU and need not be DMA-capable.
// It's aligned to make copying it more efficient.
alignas(4) IN_LOCAL_RAM
static struct {
  Pixel left_pad[16];
  Pixel buffer[max_pixels_per_line];
  Pixel right_pad[16];
} working;

static Rasterizer::RasterInfo working_buffer_shape;
static Dma::Stream::cr_value_t next_dma_xfer;
static constexpr auto dma_xfer_common = Dma::Stream::cr_value_t()
  .with_chsel(6)  // for TIM1_UP
  .with_pl(Dma::Stream::cr_value_t::pl_t::very_high)
  .with_pburst(Dma::Stream::BurstSize::single)
  .with_mburst(Dma::Stream::BurstSize::single)
  .with_en(true);
static bool scan_buffer_needs_update;

static Band const *band_list_head;
static Band current_band;
static std::atomic<bool> band_list_taken{false};


/*******************************************************************************
 * Driver API.
 */

void init() {
  // Turn on I/O compensation cell to reduce noise on power supply.
  rcc.enable_clock(ApbPeripheral::syscfg);
  syscfg.write_cmpcr(syscfg.read_cmpcr().with_cmp_pd(true));

  // Turn a bunch of stuff on.
  rcc.enable_clock(AhbPeripheral::gpiob);  // Sync signals
  rcc.enable_clock(AhbPeripheral::gpioe);  // Video
  rcc.enable_clock(AhbPeripheral::dma2);

  auto &st = dma2.stream5;

  // DMA configuration

  // Configure FIFO.
  st.write_fcr(Dma::Stream::fcr_value_t()
               .with_fth(Dma::Stream::fcr_value_t::fth_t::quarter)
               .with_dmdis(true)
               .with_feie(false));

  // Configure the pixel-generation timer used during half-horizontal mode.
  // We use TIM1; it's an APB2 (fast) peripheral, and with our clock config
  // it gets clocked at the full CPU rate.  We'll load ARR under rasterizer
  // control to synthesize 1/n rates.
  rcc.enable_clock(ApbPeripheral::tim1);
  tim1.write_psc(1 - 1);  // Divide input clock by 1.
  tim1.write_cr1(AdvTimer::cr1_value_t()
      .with_urs(true));
  tim1.write_dier(AdvTimer::dier_value_t()
      .with_ude(true));  // DRQ on update

  // Configure our interrupt priorities.  The scheme is:
  //  TIM4 (horizontal) gets highest priority.
  //  TIM3 (shock absorber) is set just lower.
  //  PendSV (rendering, user code) is lowest.
  // We could fit other stuff into the gaps later.
  // Note that PendSV is set using ARMv7-M priorities (0-255) and the others are
  // set using narrower SoC priorities (0-15).  This is a bit ugly.
  set_irq_priority(Interrupt::tim4, 0);
  set_irq_priority(Interrupt::tim3, 1);
  scb.set_exception_priority(etl::armv7m::Exception::pend_sv, 0xFF);

  // Halt all our timers on debug.
  dbg.write_dbgmcu_apb1_fz(dbg.read_dbgmcu_apb1_fz()
                           .with_dbg_tim4_stop(true)
                           .with_dbg_tim3_stop(true));

  dbg.write_dbgmcu_apb2_fz(dbg.read_dbgmcu_apb2_fz()
                           .with_dbg_tim1_stop(true));

  // Enable Flash cache and prefetching to try and reduce jitter.
  // This only affects best-effort-level code, not anything realtime.
  flash.write_acr(flash.read_acr()
                  .with_dcen(true)
                  .with_icen(true)
                  .with_prften(true));

  band_list_head = nullptr;
  band_list_taken = false;

  sync_off();
  video_off();
  arena_reset();
}

void sync_off() {
  gpiob.set_mode((1 << 6) | (1 << 7), Gpio::Mode::input);
  gpiob.set_pull((1 << 6) | (1 << 7), Gpio::Pull::down);
}

void video_off() {
  gpioe.set_mode(0xFF00, Gpio::Mode::input);
  gpioe.set_pull(0xFF00, Gpio::Pull::down);
}

void sync_on() {
  // Configure PB6 to produce hsync using TIM4_CH1
  gpiob.set_alternate_function(Gpio::p6, 2);
  gpiob.set_output_type(Gpio::p6, Gpio::OutputType::push_pull);
  gpiob.set_output_speed(Gpio::p6, Gpio::OutputSpeed::fast_50mhz);
  gpiob.set_mode(Gpio::p6, Gpio::Mode::alternate);

  // Configure PB7 as GPIO output.
  gpiob.set_output_type(Gpio::p7, Gpio::OutputType::push_pull);
  gpiob.set_output_speed(Gpio::p7, Gpio::OutputSpeed::fast_50mhz);
  gpiob.set_mode(Gpio::p7, Gpio::Mode::gpio);
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
                              GpTimer &tim) {
  rcc.enable_clock(p);
  rcc.leave_reset(p);
  tim.write_psc(2 - 1);  // Count in pixels, 1 pixel = 2 PCLK = 4 CCLK

  tim.write_arr(timing.line_pixels - 1);
  tim.write_ccr1(timing.sync_pixels);
  tim.write_ccr2(timing.sync_pixels
                 + timing.back_porch_pixels - timing.video_lead);
  tim.write_ccr3(timing.sync_pixels
                 + timing.back_porch_pixels + timing.video_pixels);

  tim.write_ccmr1(GpTimer::ccmr1_value_t()
                  .with_oc1m(GpTimer::OcMode::pwm1)
                  .with_cc1s(GpTimer::ccmr1_value_t::cc1s_t::output));

  tim.write_ccer(GpTimer::ccer_value_t()
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
  disable_h_timer(ApbPeripheral::tim4, Interrupt::tim4);
  disable_h_timer(ApbPeripheral::tim3, Interrupt::tim3);

  // Busy-wait for pending DMA to complete.
  while (dma2.stream5.read_cr().get_en());

  // Switch to new CPU clock settings.
  rcc.configure_clocks(timing.clock_config);

  // Configure TIM3/4 for horizontal sync generation.
  configure_h_timer(timing, ApbPeripheral::tim3, tim3);
  configure_h_timer(timing, ApbPeripheral::tim4, tim4);

  // Adjust tim3's CC2 value back in time.
  tim3.write_ccr2(static_cast<Word>(tim3.read_ccr2()) - 20);

  // Configure tim3 to distribute its enable signal as its trigger output.
  tim3.write_cr2(GpTimer::cr2_value_t()
                 .with_mms(GpTimer::cr2_value_t::mms_t::enable)
                 .with_ccds(false));

  // Configure tim4 to trigger from tim3 and run forever.
  tim4.write_smcr(GpTimer::smcr_value_t()
                  .with_ts(GpTimer::smcr_value_t::ts_t::itr2)
                  .with_sms(GpTimer::smcr_value_t::sms_t::trigger));

  // Turn on tim4's interrupts.
  tim4.write_dier(GpTimer::dier_value_t()
                  .with_cc2ie(true)    // Interrupt at start of active video.
                  .with_cc3ie(true));  // Interrupt at end of active video.

  // Turn on only one of tim3's
  tim3.write_dier(GpTimer::dier_value_t()
                  .with_cc2ie(true));  // Interrupt at start of active video.

  // Note: timers still not running.

  switch (timing.vsync_polarity) {
    case Timing::Polarity::positive: gpiob.clear(1 << 7); break;
    case Timing::Polarity::negative: gpiob.set  (1 << 7); break;
  }

  // Scribble over working buffer to help catch bugs.
  for (size_t i = 0; i < sizeof(working.buffer); i += 2) {
    working.buffer[i] = 0xFF;
    working.buffer[i + 1] = 0x00;
  }

  // Blank the final four pixels of the scan buffer.
  scan_buffer[timing.video_pixels + 0] = 0;
  scan_buffer[timing.video_pixels + 1] = 0;
  scan_buffer[timing.video_pixels + 2] = 0;
  scan_buffer[timing.video_pixels + 3] = 0;

  // Set up global state.
  current_line = 0;
  current_timing = timing;

  // Start TIM3, which starts TIM4.
  enable_irq(Interrupt::tim3);
  enable_irq(Interrupt::tim4);
  tim3.write_cr1(tim3.read_cr1().with_cen(true));

  sync_on();
}

void configure_band_list(Band const *head) {
  band_list_head = head;
  band_list_taken = false;
}

void clear_band_list() {
  configure_band_list(nullptr);
  while (!band_list_taken) etl::armv7m::wait_for_interrupt();
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

}  // namespace vga

void vga_hblank_interrupt()
  __attribute__((weak, alias("_ZN3vga24default_hblank_interruptEv")));

/*******************************************************************************
 * Horizontal timing interrupt.
 */

RAM_CODE
static void start_of_active_video() {
  // The start-of-active-video (SAV) event is only significant during visible
  // lines.
  if (ETL_UNLIKELY(!is_displayed_state(vga::state))) return;

  // Clear stream 5 flags (hifcr is a write-1-to-clear register).
  dma2.write_hifcr(Dma::hifcr_value_t()
                   .with_cdmeif5(true)
                   .with_cteif5(true)
                   .with_chtif5(true)
                   .with_ctcif5(true));

  // Start the countdown for first DRQ.
  tim1.write_cr1(AdvTimer::cr1_value_t()
      .with_urs(true)
      .with_cen(true));

  dma2.stream5.write_cr(vga::next_dma_xfer);
}

RAM_CODE
static void end_of_active_video() {
  // The end-of-active-video (EAV) event is always significant, as it advances
  // the line state machine and kicks off PendSV.

  // Shut off TIM1; only really matters in half-horizontal mode.
  tim1.write_cr1(AdvTimer::cr1_value_t()
      .with_urs(true)
      .with_cen(false));

  vga::Timing const &timing = vga::current_timing;

  // Apply timing changes requested by the last rasterizer.
  tim4.write_ccr2(timing.sync_pixels
                  + timing.back_porch_pixels - timing.video_lead
                  + vga::working_buffer_shape.offset);

  // Pend a PendSV to process hblank tasks.
  scb.write_icsr(Scb::icsr_value_t().with_pendsvset(true));

  // We've finished this line; figure out what to do on the next one.
  unsigned next_line = vga::current_line + 1;

  if (next_line == timing.vsync_start_line
      || next_line == timing.vsync_end_line) {
    // Either edge of vsync pulse.
    gpiob.toggle(Gpio::p7);
  } else if (next_line == uint16_t(timing.video_start_line - 1)) {
    // We're one line before scanout begins -- need to start rasterizing.
    vga::state = vga::State::starting;
    if (vga::band_list_head) {
      vga::current_band = *vga::band_list_head;
    } else {
      vga::current_band = { nullptr, 0, nullptr };
    }
    vga::band_list_taken = true;
  } else if (next_line == timing.video_start_line) {
    // Time to start output.  This will cause PendSV to copy rasterization
    // output into place for scanout, and the next SAV will start DMA.
    vga::state = vga::State::active;
  } else if (next_line == uint16_t(timing.video_end_line - 1)) {
    // For the final line, suppress rasterization but continue preparing
    // previously rasterized data for scanout, and continue starting DMA in
    // SAV.
    vga::state = vga::State::finishing;
  } else if (next_line == uint16_t(timing.video_end_line)) {
    // All done!  Suppress all scanout activity.
    vga::state = vga::State::blank;
    next_line = 0;
  }

  vga::current_line = next_line;
}

RAM_CODE void etl_stm32f4xx_tim3_handler() {
  // We access this APB2 timer through the bridge on AHB1.  This implies
  // both wait states and resource conflicts with scanout.  Get done fast.
  tim3.write_sr(tim3.read_sr().with_cc2if(false));

  // Idle the processor until preempted by any higher-priority interrupt.
  // This ensures that the M4's D-code bus is available for exception entry.
  // NOTE: this behaves correctly on the M4, but WFI is not guaranteed to
  // actually do anything.
  etl::armv7m::wait_for_interrupt();
}

RAM_CODE void etl_stm32f4xx_tim4_handler() {
  // We have to clear our interrupt flags, or this will recur.
  auto sr = tim4.read_sr();

  if (ETL_LIKELY(sr.get_cc2if())) {
    tim4.write_sr(sr.with_cc2if(false));
    start_of_active_video();
    return;
  }

  if (sr.get_cc3if()) {
    tim4.write_sr(sr.with_cc3if(false));
    end_of_active_video();
    return;
  }
}

RAM_CODE
static bool advance_rasterizer_band(bool edge = false) {
  if (vga::current_band.line_count) {
    --vga::current_band.line_count;
    return edge;
  }

  if (vga::current_band.next) {
    vga::current_band = *vga::current_band.next;
    return advance_rasterizer_band(true);
  } else {
    return edge;
  }
}

RAM_CODE
static vga::Rasterizer *get_rasterizer() {
  return vga::current_band.rasterizer;
}

RAM_CODE
void etl_armv7m_pend_sv_handler() {
  // PendSV event is triggered shortly after EAV to process lower-priority
  // tasks.

  // First, prepare for scanout from SAV on this line.  This has two purposes:
  // it frees up the rasterization target buffer so that we can overwrite it,
  // and it applies pixel timing choices from the *last* rasterizer run to the
  // scanout machine so that we can replace them as well.
  //
  // This writes to the scanout buffer *and* accesses AHB/APB peripherals, so it
  // *cannot* run concurrently with scanout -- so we do it first, during hblank.
  if (ETL_LIKELY(is_displayed_state(vga::state))) {
    if (vga::scan_buffer_needs_update) {
      // Flip working_buffer into scan_buffer.  We know its contents are ready
      // because of the scan_buffer_needs_update flag.  Note that the flag may
      // not have been set, even in a displayed state, if we're repeating a
      // line.
      //
      // Note that GCC can't see that we've aligned the buffers correctly, so we
      // have to do a multi-cast dance. :-/
      ((Word *) (void *) vga::scan_buffer)[
          vga::working_buffer_shape.length / 4] = 0;
      copy_words(
          reinterpret_cast<Word const *>(
            static_cast<void *>(vga::working.buffer)),
          reinterpret_cast<Word *>(
            static_cast<void *>(vga::scan_buffer)),
          vga::working_buffer_shape.length / 4);
      vga::scan_buffer_needs_update = false;
    }

    auto & st = dma2.stream5;
    if (vga::working_buffer_shape.stretch_cycles) {
      // Adjust reload frequency of TIM1 to accomodate desired pixel clock.
      tim1.write_arr(vga::working_buffer_shape.stretch_cycles + 4 - 1);
      // Force an update to reset the timer state.
      tim1.write_egr(AdvTimer::egr_value_t().with_ug(true));
      // Nudge the count down to delay the first DRQ (fudge factor)
      tim1.write_cnt(uint32_t(-4));

      st.write_par(0x40021015);  // High byte of GPIOE ODR (hack hack)
      st.write_m0ar(reinterpret_cast<Word>(&vga::scan_buffer));
      st.write_ndtr(vga::working_buffer_shape.length + 4);

      vga::next_dma_xfer = vga::dma_xfer_common
          .with_dir(Dma::Stream::cr_value_t::dir_t::memory_to_peripheral)
          .with_msize(Dma::Stream::TransferSize::word)
          .with_minc(true)
          .with_psize(Dma::Stream::TransferSize::byte)
          .with_pinc(false);
    } else {
      st.write_ndtr(vga::working_buffer_shape.length / 4 + 1);
      // Note that we're using memory as the peripheral side.
      // This DMA controller is a little odd.
      st.write_par(reinterpret_cast<Word>(&vga::scan_buffer));
      st.write_m0ar(0x40021015);  // High byte of GPIOE ODR (hack hack)

      vga::next_dma_xfer = vga::dma_xfer_common
          .with_dir(Dma::Stream::cr_value_t::dir_t::memory_to_memory)
          .with_psize(Dma::Stream::TransferSize::word)
          .with_pinc(true)
          .with_msize(Dma::Stream::TransferSize::byte)
          .with_minc(false);
    }
  }

  // Allow the application to do additional work during what's left of hblank.
  vga_hblank_interrupt();

  // Second, rasterize the *next* line, if there's a useful next line.
  // Rasterization can take a while, and may run concurrently with scanout.
  // As a result, we just stash our results in places where the *next* PendSV
  // will find and apply them.
  if (ETL_LIKELY(is_rendered_state(vga::state))) {
    auto const &timing = vga::current_timing;
    auto next_line = vga::current_line + 1;
    auto visible_line = next_line - timing.video_start_line;

    bool band_edge = advance_rasterizer_band();
    if (vga::working_buffer_shape.repeat_lines == 0 || band_edge) {
      auto r = get_rasterizer();
      if (r) {
        vga::working_buffer_shape = r->rasterize(visible_line,
                                                 vga::working.buffer);
        // Request a rewrite of the scanout buffer during next hblank.
        vga::scan_buffer_needs_update = true;
      }
    } else {  // repeat_lines > 0, not band_edge
      --vga::working_buffer_shape.repeat_lines;
    }
  }
}
