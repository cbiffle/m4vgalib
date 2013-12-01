#ifndef VGA_VGA_H
#define VGA_VGA_H

#include "vga/mode.h"

namespace vga {

void init();

void video_on();
void video_off();

void select_mode(Mode *);

}  // namespace vga

#endif  // VGA_VGA_H
