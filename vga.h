#ifndef VGA_VGA_H
#define VGA_VGA_H

#include "etl/types.h"

namespace vga {

typedef etl::UInt8 Pixel;

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

unsigned get_cpu_usage();

}  // namespace vga

/*
 * Applications can implement this function to receive a callback during hblank.
 */
extern void vga_hblank_interrupt();

#endif  // VGA_VGA_H
