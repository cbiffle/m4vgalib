#include "vga/measurement.h"

#include "etl/stm32f4xx/ahb.h"
#include "etl/stm32f4xx/rcc.h"

using etl::armv7m::SysTick;
using etl::armv7m::sys_tick;

using etl::stm32f4xx::AhbPeripheral;
using etl::stm32f4xx::Gpio;
using etl::stm32f4xx::gpioc;
using etl::stm32f4xx::rcc;

namespace vga {

void msigs_init() {
  rcc.enable_clock(AhbPeripheral::gpioc);

  gpioc.set_output_type(Gpio::p8 | Gpio::p9, Gpio::OutputType::push_pull);
  gpioc.set_output_speed(Gpio::p8 | Gpio::p9, Gpio::OutputSpeed::high_100mhz);
  gpioc.set_mode(Gpio::p8 | Gpio::p9, Gpio::Mode::gpio);
}

void mtim_init() {
  sys_tick.write_rvr(0xFFFFFF);
  sys_tick.write_csr(SysTick::csr_value_t()
                     .with_clksource(
                        SysTick::csr_value_t::clksource_t::cpu_clock)
                     .with_tickint(false)
                     .with_enable(true));
}

}  // namespace vga
