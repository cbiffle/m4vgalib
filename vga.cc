#include "vga/vga.h"

#include "lib/armv7m/exceptions.h"

#include "lib/stm32f4xx/ahb.h"
#include "lib/stm32f4xx/apb.h"
#include "lib/stm32f4xx/gpio.h"
#include "lib/stm32f4xx/interrupts.h"
#include "lib/stm32f4xx/rcc.h"

using stm32f4xx::AhbPeripheral;
using stm32f4xx::ApbPeripheral;
using stm32f4xx::Gpio;
using stm32f4xx::gpioc;
using stm32f4xx::gpioe;
using stm32f4xx::Interrupt;
using stm32f4xx::rcc;

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

}  // namespace vga
