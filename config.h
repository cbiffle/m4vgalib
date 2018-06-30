#ifndef VGA_CONFIG_H
#define VGA_CONFIG_H

// This can be used to change pins for RGB
#define VIDEO_GPIO gpioe
#define VIDEO_GPIO_MASK 0xFF00
#define VIDEO_GPIO_ODR 0x40021015
//#define VIDEO_GPIO gpioa
//#define VIDEO_GPIO_MASK 0x003F
//#define VIDEO_GPIO_ODR ((Word)&GPIOA->ODR)

// This is useful if I would like to use HAL for interrupts
#define IRQ
//#define IRQ extern "C"

// This is useful if I don't use arena.h
#include "vga/arena.h"
//#define arena_reset() 

#endif
