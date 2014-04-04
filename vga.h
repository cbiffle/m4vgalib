#ifndef VGA_VGA_H
#define VGA_VGA_H

#include "etl/common/types.h"

namespace vga {

typedef etl::common::UInt8 Pixel;

struct Timing;
class Rasterizer;

void init();

void video_on();
void video_off();

void sync_off();
void sync_on();

void configure_timing(Timing const &);

struct Band {
  Rasterizer *rasterizer;
  unsigned line_count;
  Band const *next;
};

void configure_band_list(Band const *head);

void wait_for_vblank();

bool in_vblank();

void sync_to_vblank();

/*
 * Applications can implement this function to receive a callback during hblank.
 */
extern void hblank_interrupt();

}  // namespace vga

#endif  // VGA_VGA_H
