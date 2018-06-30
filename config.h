#ifndef VGA_CONFIG_H
#define VGA_CONFIG_H

// This is used to change pins for RGB
#define VIDEO_GPIO gpioa
#define VIDEO_GPIO_MASK 0x003F
#define VIDEO_GPIO_ODR 0x40021015

// This is useful if I would like to use HAL for interrupts
#define IRQ
//#define IRQ extern "C"

// This is useful if I don't use arena.harderr
#include "vga/arena.h"
//#define arena_reset() 

#endif
