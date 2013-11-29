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

  // Start the timer.
  enable_irq(Interrupt::tim8_cc);
  tim8.write_cr1(tim8.read_cr1().with_cen(true));

  // Enable display and sync signals.
  video_on();
}

}  // namespace vga
