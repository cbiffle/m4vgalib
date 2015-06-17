#ifndef VGA_MEASUREMENT_H
#define VGA_MEASUREMENT_H

#include "etl/attribute_macros.h"
#include "etl/armv7m/sys_tick.h"
#include "etl/stm32f4xx/gpio.h"

namespace vga {

/*******************************************************************************
 * SysTick timer profiling support.
 *
 * The SysTick timer can be configured to run at the CPU clock frequency, so we
 * can read it to derive reasonably precise cycle counts for operations.  This
 * is valuable for profiling sequences of instructions.
 *
 * Note that this is only safe if no other software (like an operating system)
 * expects to use the SysTick timer.
 */

/*
 * Configure the SysTick timer for profiling use.
 */
void mtim_init();

/*
 * Read the cycle counter.  Note that this is a 24-bit down counter!
 */
ETL_INLINE unsigned mtim_get() {
  return etl::armv7m::sys_tick.read_cvr().get_current();
}

/*******************************************************************************
 * GPIO profiling support.
 *
 * These operations are designed to time events inside a program by detecting
 * external pin transitions using an oscilloscope or logic analyzer.
 *
 * Toggling GPIOs requires AHB writes, and can disrupt the flow of video.  To
 * avoid this, all GPIO-related profiling is compiled out unless the build
 * environment defines DISRUPTIVE_MEASUREMENT.
 */

/*
 * Configures some GPIOs for profiling use.
 */
void msigs_init();

#ifdef DISRUPTIVE_MEASUREMENT

ETL_INLINE void msig_a_set() {
  etl::stm32f4xx::gpioc.set(etl::stm32f4xx::Gpio::p9);
}

ETL_INLINE void msig_a_clear() {
  etl::stm32f4xx::gpioc.clear(etl::stm32f4xx::Gpio::p9);
}

ETL_INLINE void msig_a_toggle() {
  etl::stm32f4xx::gpioc.toggle(etl::stm32f4xx::Gpio::p9);
}

ETL_INLINE void msig_b_set() {
  etl::stm32f4xx::gpioc.set(etl::stm32f4xx::Gpio::p8);
}

ETL_INLINE void msig_b_clear() {
  etl::stm32f4xx::gpioc.clear(etl::stm32f4xx::Gpio::p8);
}

ETL_INLINE void msig_b_toggle() {
  etl::stm32f4xx::gpioc.toggle(etl::stm32f4xx::Gpio::p8);
}

ETL_INLINE void msig_e_set(unsigned index) {
  etl::stm32f4xx::gpioe.set((1 << index) & 0xFF);
}

ETL_INLINE void msig_e_clear(unsigned index) {
  etl::stm32f4xx::gpioe.clear((1 << index) & 0xFF);
}

#else

ETL_INLINE void msig_a_set() {}

ETL_INLINE void msig_a_clear() {}

ETL_INLINE void msig_a_toggle() {}

ETL_INLINE void msig_b_set() {}

ETL_INLINE void msig_b_clear() {}

ETL_INLINE void msig_b_toggle() {}

ETL_INLINE void msig_e_set(unsigned index) {}

ETL_INLINE void msig_e_clear(unsigned index) {}

#endif

}  // namespace vga

#endif  // VGA_MEASUREMENT_H
