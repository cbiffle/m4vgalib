#include "vga/vga.h"

#include "lib/armv7m/exceptions.h"

#include "lib/stm32f4xx/adv_timer.h"
#include "lib/stm32f4xx/ahb.h"
#include "lib/stm32f4xx/apb.h"
#include "lib/stm32f4xx/gpio.h"
#include "lib/stm32f4xx/interrupts.h"
#include "lib/stm32f4xx/rcc.h"

using stm32f4xx::AdvTimer;
using stm32f4xx::AhbPeripheral;
using stm32f4xx::ApbPeripheral;
using stm32f4xx::Gpio;
using stm32f4xx::gpioc;
using stm32f4xx::gpioe;
using stm32f4xx::Interrupt;
using stm32f4xx::rcc;
using stm32f4xx::tim8;

namespace vga {

static unsigned volatile current_line;
static VideoMode const *current_mode;

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

static State volatile state;

void init() {
  // TODO(cbiffle): original code turned on I/O compensation.  Should we?

  // Turn a bunch of stuff on.
  rcc.enable_clock(ApbPeripheral::tim8);
  rcc.enable_clock(AhbPeripheral::gpioc);  // Sync signals
  rcc.enable_clock(AhbPeripheral::gpioe);  // Video
  rcc.enable_clock(AhbPeripheral::dma2);

  // Hold TIM8 in reset until we do mode-setting.
  rcc.enter_reset(ApbPeripheral::tim8);

  set_irq_priority(Interrupt::tim8_cc, 0);
  set_exception_priority(armv7m::Exception::pend_sv, 0xFF);

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
  gpioc.set_output_speed(Gpio::p6, Gpio::OutputSpeed::high_100mhz);
  gpioc.set_mode(Gpio::p6, Gpio::Mode::alternate);

  // Configure PC7 as GPIO output.
  gpioc.set_output_type(Gpio::p7, Gpio::OutputType::push_pull);
  gpioc.set_output_speed(Gpio::p7, Gpio::OutputSpeed::high_100mhz);
  gpioc.set_mode(Gpio::p7, Gpio::Mode::gpio);

  // Configure the high byte of port E for parallel video.
  gpioe.set_output_type(0xFF00, Gpio::OutputType::push_pull);
  gpioe.set_output_speed(0xFF00, Gpio::OutputSpeed::high_100mhz);
  gpioe.set_mode(0xFF00, Gpio::Mode::gpio);
}

void select_mode(VideoMode const &mode) {
  // Disable outputs during mode change.
  video_off();

  // Place TIM8 in reset, stopping all timing, and disable its interrupt.
  rcc.enter_reset(ApbPeripheral::tim8);
  disable_irq(Interrupt::tim8_cc);

  // TODO: wait for completion of DMA.

  // TODO: apply buffered changes.

  // Switch to new CPU clock settings.
  rcc.configure_clocks(mode.clock_config);

  // Configure TIM8 for horizontal sync generation.
  rcc.leave_reset(ApbPeripheral::tim8);
  tim8.write_psc(4 - 1);  // Count in pixels, 1 pixel = 4 CCLK

  tim8.write_arr(mode.line_pixels - 1);
  tim8.write_ccr1(mode.sync_pixels);
  tim8.write_ccr2(mode.sync_pixels + mode.back_porch_pixels - mode.video_lead);
  tim8.write_ccr3(
      mode.sync_pixels + mode.back_porch_pixels + mode.video_pixels);

  tim8.write_ccmr1(AdvTimer::ccmr1_value_t()
                   .with_oc1m(AdvTimer::OcMode::pwm1)
                   .with_cc1s(AdvTimer::ccmr1_value_t::cc1s_t::output));

  tim8.write_bdtr(AdvTimer::bdtr_value_t()
                  .with_ossr(true)
                  .with_moe(true));

  tim8.write_ccer(AdvTimer::ccer_value_t()
                  .with_cc1e(true)
                  .with_cc1p(
                      mode.hsync_polarity == VideoMode::Polarity::negative));

  tim8.write_dier(AdvTimer::dier_value_t()
                  .with_cc2ie(true)    // Interrupt at start of active video.
                  .with_cc3ie(true));  // Interrupt at end of active video.

  // Note: TIM8 is still not running.

  switch (mode.vsync_polarity) {
    case VideoMode::Polarity::positive: gpioc.clear(1 << 7); break;
    case VideoMode::Polarity::negative: gpioc.set  (1 << 7); break;
  }

  // TODO: scribble scan buffer to help catch bugs.

  // TODO: ensure final pixels are black.

  // Set up global state.
  current_mode = &mode;  // TODO(cbiffle): copy into RAM for less jitter?
  current_line = 0;

  // Start the timer.
  enable_irq(Interrupt::tim8_cc);
  tim8.write_cr1(tim8.read_cr1().with_cen(true));

  // Enable display and sync signals.
  video_on();
}

}  // namespace vga

extern "C" void stm32f4xx_tim8_cc_handler() {
  // We have to clear our interrupt flags, or this will recur.
  auto sr = tim8.read_sr();
  tim8.write_sr(sr.with_cc2if(false).with_cc3if(false));

  if (sr.get_cc2if()) {
    // CC2 indicates start of active video.
    // TODO(cbiffle): kick off DMA here.

    // hack hack
    if (is_displayed_state(vga::state)) {
      gpioe.set(0xFF00);
    }
  }

  if (sr.get_cc3if()) {
    // CC3 indicates end of active video.
    unsigned line = vga::current_line;
    vga::VideoMode const &mode = *vga::current_mode;

    // hack hack
    gpioe.clear(0xFF00);

    if (line == 0) {
      // Start of frame!  Time to stop displaying pixels.
      vga::state = vga::State::blank;
      // TODO(cbiffle): suppress video output.
      // TODO(cbiffle): latch configuration changes.
    } else if (line == mode.vsync_start_line
            || line == mode.vsync_end_line) {
      // Either edge of vsync pulse.
      gpioc.toggle(Gpio::p7);
    } else if (line == static_cast<unsigned short>(mode.video_start_line - 1)) {
      // Time to start generating the first scan buffer.
      vga::state = vga::State::starting;
    } else if (line == mode.video_start_line) {
      // Time to start output.
      vga::state = vga::State::active;
    } else if (line == static_cast<unsigned short>(mode.video_end_line - 1)) {
      // Time to stop rendering new scan buffers.
      vga::state = vga::State::finishing;
      line = static_cast<unsigned>(-1);  // Make line roll over to zero.
    }

    vga::current_line = line + 1;

    if (is_rendered_state(vga::state)) {
      // TODO(cbiffle): PendSV to kick off a buffer flip and rasterization.
    }
  }
}

