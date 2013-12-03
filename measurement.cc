#include "vga/measurement.h"

using stm32f4xx::Gpio;
using stm32f4xx::gpioc;

namespace vga {

void msigs_init() {
  gpioc.set_output_type(Gpio::p9, Gpio::OutputType::push_pull);
  gpioc.set_output_speed(Gpio::p9, Gpio::OutputSpeed::high_100mhz);
  gpioc.set_mode(Gpio::p9, Gpio::Mode::gpio);
}

}  // namespace vga
