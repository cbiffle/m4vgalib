#include "vga/vga.h"

#include <atomic>
#include <cstddef>
#include <cstdint>

#include "etl/assert.h"
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

using etl::armv7m::Byte;
using etl::armv7m::HalfWord;
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
 * Driver configuration.
 */

static constexpr unsigned
  // Used to adjust size of scan_buffer.
  max_pixels_per_line = 800,
  // Fudge factor: shifts timer-initiated DRQ back in time by this many cycles,
  // to delay DRQ until DMA has started.
  drq_shift_cycles = 2,
  // Fudge factor: how long the shock absorber IRQ should lead the actual start
  // of video IRQ, in cycles.
  shock_absorber_shift_cycles = 20,
  // Amount of pad to place on either side of the working buffer, so that lazy
  // rasterizers can scribble slightly outside the lines -- in words.
  extra_pad_words = 4;

// Common fields used in scanout DMA transfer settings.
static constexpr auto dma_xfer_common = Dma::Stream::cr_value_t()
  .with_chsel(6)  // for TIM1_UP
  .with_pl(Dma::Stream::cr_value_t::pl_t::very_high)
  .with_pburst(Dma::Stream::BurstSize::single)
  .with_mburst(Dma::Stream::BurstSize::single)
  .with_en(true);


/*******************************************************************************
 * Driver state.
 */

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

// This is the DMA source for scan-out, copied from the working buffer during
// pend_sv.  It must be located in DMA-capable RAM, and is aligned to allow for
// word-sized DMA reads.
//
// It contains an extra word's worth of pixels to ensure that we can follow
// every line with an extra transfer to blank the outputs.  The extra pixels
// are blanked after the rasterizer returns.
alignas(Word) IN_SCAN_RAM
static Pixel scan_buffer[max_pixels_per_line + sizeof(Word)];

// This is the working buffer, the target of the Rasterizer.  Its contents will
// be copied to the scan_buffer during hblank if needed.  It need not be in
// DMA-capable RAM.
//
// It's aligned so we can use a high-speed word copy routine.
//
// It has invisible padding at either end because it makes certain tile
// scrolling algorithms simpler to implement if they need not color precisely
// within the lines.
alignas(Word) IN_LOCAL_RAM
static struct {
  Word left_pad[extra_pad_words];
  Pixel buffer[max_pixels_per_line];
  Word right_pad[extra_pad_words];
} working;

// A description of the contents of the working buffer, produced by the last
// Rasterizer that was applied.  This is used to adjust the output timings.
static Rasterizer::RasterInfo working_buffer_shape;

// When a Rasterizer completes and updates the working buffer, we set this
// flag.  This triggers a copy into the scan buffer at next hblank, at which
// time the flag is cleared.  The copy is conditional because it isn't always
// necessary, e.g. in rasterizer that use line-doubling, so we can save some
// resources by eliding it.
//
// Since this is produced and consumed by interrupts running at a single
// priority, it need not be volatile or atomic.
static bool scan_buffer_needs_update;

// A pre-built control register word to be used to start the next DMA transfer.
// This is set up during hblank based on the working_buffer_shape, and consumed
// at start of active video.
static Dma::Stream::cr_value_t next_dma_xfer;
IN_LOCAL_RAM
static bool next_use_timer;

// The head of the linked list of Rasterizer bands.
static Band const *band_list_head;

// A copy of the band we're currently processing.  We copy for several reasons:
// - So that the application may keep its Bands in Flash without a latency
//   penalty on the driver.
// - So that we may mutate it, decrementing the line count.
// - So that the application may rewrite its Bands once rendering starts.
static Band current_band;

// A semaphore used to indicate, to the application, when the driver has
// begun processing the most recently configured band list.  Because the
// driver maintains a copy of a Band, and this copy contains a pointer, it
// is not safe to deallocate or repurpose a list of Bands while the driver
// may be using them.  Instead, clear_band_list does it safely using this
// semaphore.
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

  // Configure the pixel-generation timer used during reduced-horizontal mode.
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

  // Configure the timer to count in pixels.  These timers live on APB1.
  // Like all APB timers they get their clocks doubled at certain APB
  // multipliers.
  auto apb_cycles_per_pixel = timing.clock_config.apb1_divisor > 1
      ? (timing.cycles_per_pixel * 2 / timing.clock_config.apb1_divisor)
      : timing.cycles_per_pixel;

  tim.write_psc(apb_cycles_per_pixel - 1);

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

  // No scanout strategy can achieve fewer than 4 cycles per pixel.
  ETL_ASSERT(timing.cycles_per_pixel >= 4);
  // Because horizontal timing is managed by timers on the slower APB1 bus,
  // make sure that we can express the (AHB) cycles_per_pixel in APB1 units.
  if (timing.clock_config.apb1_divisor > 1) {
    ETL_ASSERT(timing.cycles_per_pixel % (timing.clock_config.apb1_divisor / 2)
                  == 0);
  }

  // Switch to new CPU clock settings.
  rcc.configure_clocks(timing.clock_config);

  // Configure TIM3/4 for horizontal sync generation.
  configure_h_timer(timing, ApbPeripheral::tim3, tim3);
  configure_h_timer(timing, ApbPeripheral::tim4, tim4);

  // Adjust tim3's CC2 value back in time.
  tim3.write_ccr2(Word(tim3.read_ccr2()) - shock_absorber_shift_cycles);

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

  // Blank the final word of the scan buffer.
  for (unsigned i = 0; i < sizeof(Word); ++i) {
    scan_buffer[timing.video_pixels + i] = 0;
  }

  // Set up global state.
  current_line = 0;
  current_timing = timing;
  state = State::blank;
  working_buffer_shape = {
    .offset = 0,
    .length = 0,
    .cycles_per_pixel = timing.cycles_per_pixel,
    .repeat_lines = 0,
  };
  next_use_timer = false;

  scan_buffer_needs_update = false;

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

/*******************************************************************************
 * Horizontal timing implementation.  See also the ISR, declared outside of
 * namespace vga toward the end of the file.
 */

RAM_CODE
static void start_of_active_video() {
  // The start-of-active-video (SAV) event is only significant during visible
  // lines.
  if (ETL_UNLIKELY(!is_displayed_state(state))) return;

  // Clear stream 5 flags (hifcr is a write-1-to-clear register).
  dma2.write_hifcr(Dma::hifcr_value_t()
                   .with_cdmeif5(true)
                   .with_cteif5(true)
                   .with_chtif5(true)
                   .with_ctcif5(true));

  // Start the countdown for first DRQ.
  tim1.write_cr1(AdvTimer::cr1_value_t()
      .with_urs(true)
      .with_cen(next_use_timer));

  dma2.stream5.write_cr(next_dma_xfer);
}

RAM_CODE
static void end_of_active_video() {
  // The end-of-active-video (EAV) event is always significant, as it advances
  // the line state machine and kicks off PendSV.

  // Shut off TIM1; only really matters in reduced-horizontal mode.
  tim1.write_cr1(AdvTimer::cr1_value_t()
      .with_urs(true)
      .with_cen(false));

  // Apply timing changes requested by the last rasterizer.
  tim4.write_ccr2(current_timing.sync_pixels
                  + current_timing.back_porch_pixels - current_timing.video_lead
                  + working_buffer_shape.offset);

  // Pend a PendSV to process hblank tasks.
  scb.write_icsr(Scb::icsr_value_t().with_pendsvset(true));

  // We've finished this line; figure out what to do on the next one.
  unsigned next_line = current_line + 1;

  if (next_line == current_timing.vsync_start_line
      || next_line == current_timing.vsync_end_line) {
    // Either edge of vsync pulse.
    gpiob.toggle(Gpio::p7);
  } else if (next_line == uint16_t(current_timing.video_start_line - 1)) {
    // We're one line before scanout begins -- need to start rasterizing.
    state = State::starting;
    if (band_list_head) {
      current_band = *band_list_head;
    } else {
      current_band = { nullptr, 0, nullptr };
    }
    band_list_taken = true;
  } else if (next_line == current_timing.video_start_line) {
    // Time to start output.  This will cause PendSV to copy rasterization
    // output into place for scanout, and the next SAV will start DMA.
    state = State::active;
  } else if (next_line == uint16_t(current_timing.video_end_line - 1)) {
    // For the final line, suppress rasterization but continue preparing
    // previously rasterized data for scanout, and continue starting DMA in
    // SAV.
    state = State::finishing;
  } else if (next_line == uint16_t(current_timing.video_end_line)) {
    // All done!  Suppress all scanout activity.
    state = State::blank;
    next_line = 0;
  }

  current_line = next_line;
}

void default_hblank_interrupt();  // decl hack
RAM_CODE void default_hblank_interrupt() {}


/*******************************************************************************
 * Rasterization interface.  These are implementation factors of the PendSV
 * ISR.
 */

/*
 * Advances the current rasterizer band, possibly switching it for the next if
 * we've reached the end.  The 'edge' parameter is used only in the recursive
 * case.  (It would not appear at all if this language had nested functions.)
 */
RAM_CODE
static bool advance_rasterizer_band(bool edge = false) {
  if (current_band.line_count) {
    --current_band.line_count;
    return edge;
  }

  if (current_band.next) {
    current_band = *current_band.next;
    return advance_rasterizer_band(true);
  } else {
    current_band = { nullptr, 0, nullptr };
    return edge;
  }
}

/*
 * Transfers the contents of the working buffer into the scan buffer, if
 * necessary.
 */
RAM_CODE
static void update_scan_buffer() {
  if (scan_buffer_needs_update) {
    // Flip working_buffer into scan_buffer.  We know its contents are ready
    // because of the scan_buffer_needs_update flag.  Note that the flag may
    // not have been set, even in a displayed state, if we're repeating a
    // line.
    //
    // Note that GCC can't see that we've aligned the buffers correctly, so we
    // have to do a multi-cast dance. :-/
    copy_words(
        reinterpret_cast<Word const *>(
          static_cast<void *>(working.buffer)),
        reinterpret_cast<Word *>(
          static_cast<void *>(scan_buffer)),
        (working_buffer_shape.length + sizeof(Word) - 1) / sizeof(Word));
    for (unsigned i = 0; i < sizeof(Word); ++i) {
      scan_buffer[working_buffer_shape.length + i] = 0;
    }
    scan_buffer_needs_update = false;
  }
}

/*
 * Prepares a configuration for the DMA stream and configures the horizontal
 * timer, if it's relevant to this mode.
 */
RAM_CODE
static void prepare_for_scanout() {
  auto & st = dma2.stream5;
  st.write_cr(st.read_cr().with_en(false));

  if (working_buffer_shape.cycles_per_pixel > 4) {
    // Adjust reload frequency of TIM1 to accomodate desired pixel clock.
    // (ARR value is period - 1.)
    tim1.write_arr(working_buffer_shape.cycles_per_pixel - 1);
    // Force an update to reset the timer state.
    tim1.write_egr(AdvTimer::egr_value_t().with_ug(true));
    // Configure the timer as *almost* ready to produce a DRQ, less a small
    // value (fudge factor).  Gotta do this after the update event, above,
    // because that clears CNT.
    tim1.write_cnt(uint32_t(tim1.read_arr()) - drq_shift_cycles);
    tim1.write_sr(0);

    st.write_par(0x40021015);  // High byte of GPIOE ODR (hack hack)
    st.write_m0ar(reinterpret_cast<Word>(&scan_buffer));

    // The number of bytes read must exactly match the number of bytes written,
    // or the DMA controller will freak out.  Thus, we must adapt the transfer
    // size to the number of bytes transferred.
    Dma::Stream::TransferSize msize;
    switch (working_buffer_shape.length & 3) {
      case 0:
        msize = Dma::Stream::TransferSize::word;
        st.write_ndtr(working_buffer_shape.length + sizeof(Word));
        break;

      case 2:
        msize = Dma::Stream::TransferSize::half_word;
        st.write_ndtr(working_buffer_shape.length + sizeof(HalfWord));
        break;

      default:
        msize = Dma::Stream::TransferSize::byte;
        st.write_ndtr(working_buffer_shape.length + sizeof(Byte));
        break;
    }

    next_dma_xfer = dma_xfer_common
        .with_dir(Dma::Stream::cr_value_t::dir_t::memory_to_peripheral)
        .with_msize(msize)
        .with_minc(true)
        .with_psize(Dma::Stream::TransferSize::byte)
        .with_pinc(false);
    next_use_timer = true;

  } else {
    // Note that we're using memory as the peripheral side.
    // This DMA controller is a little odd.
    st.write_par(reinterpret_cast<Word>(&scan_buffer));
    st.write_m0ar(0x40021015);  // High byte of GPIOE ODR (hack hack)

    Dma::Stream::TransferSize psize;
    switch (working_buffer_shape.length & 3) {
      case 0:
        psize = Dma::Stream::TransferSize::word;
        st.write_ndtr(working_buffer_shape.length / sizeof(Word) + 1);
        break;

      case 2:
        psize = Dma::Stream::TransferSize::half_word;
        st.write_ndtr(working_buffer_shape.length / sizeof(HalfWord) + 1);
        break;

      default:
        psize = Dma::Stream::TransferSize::byte;
        st.write_ndtr(working_buffer_shape.length / sizeof(Byte) + 1);
        break;
    }

    next_dma_xfer = dma_xfer_common
        .with_dir(Dma::Stream::cr_value_t::dir_t::memory_to_memory)
        .with_psize(psize)
        .with_pinc(true)
        .with_msize(Dma::Stream::TransferSize::byte)
        .with_minc(false);
    next_use_timer = false;
  }
}

/*
 * Generates pixels for the *next* line, not the currently displaying one.
 */
RAM_CODE
static void rasterize_next_line() {
  auto const &timing = current_timing;
  auto next_line = current_line + 1;
  auto visible_line = next_line - timing.video_start_line;

  bool band_edge = advance_rasterizer_band();
  if (working_buffer_shape.repeat_lines == 0 || band_edge) {
    // Either the last rasterizer has run out of its repeat count and wants
    // to be called again, or we've reached a band edge and are going to call
    // the new rasterizer no matter what the old one wished.
    auto r = current_band.rasterizer;
    if (r) {
      working_buffer_shape = r->rasterize(current_timing.cycles_per_pixel,
                                          visible_line,
                                          working.buffer);
      // Request a rewrite of the scanout buffer during next hblank.
    } else {
      working_buffer_shape = {
        .offset = 0,
        .length = 0,
        .cycles_per_pixel = current_timing.cycles_per_pixel,
        .repeat_lines = 0,
      };
    }
    scan_buffer_needs_update = true;
  } else {  // repeat_lines > 0, not band_edge
    --working_buffer_shape.repeat_lines;
  }
}

}  // namespace vga


/*******************************************************************************
 * ISRs and user interrupt hook
 */

void vga_hblank_interrupt()
  __attribute__((weak, alias("_ZN3vga24default_hblank_interruptEv")));

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
    vga::start_of_active_video();
    return;
  }

  if (sr.get_cc3if()) {
    tim4.write_sr(sr.with_cc3if(false));
    vga::end_of_active_video();
    return;
  }
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
    vga::update_scan_buffer();
    vga::prepare_for_scanout();
  }

  // Allow the application to do additional work during what's left of hblank.
  vga_hblank_interrupt();

  // Second, rasterize the *next* line, if there's a useful next line.
  // Rasterization can take a while, and may run concurrently with scanout.
  // As a result, we just stash our results in places where the *next* PendSV
  // will find and apply them.
  if (ETL_LIKELY(is_rendered_state(vga::state))) {
    vga::rasterize_next_line();
  }
}
