#ifndef VGA_VGA_H
#define VGA_VGA_H

#include "vga/mode.h"

namespace vga {

void init();

void video_on();
void video_off();

void select_mode(Mode *);

void wait_for_vblank();

bool in_vblank();

void sync_to_vblank();

}  // namespace vga

#endif  // VGA_VGA_H
