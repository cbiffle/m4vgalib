#ifndef VGA_VGA_H
#define VGA_VGA_H

namespace vga {

struct Timing;
class Rasterizer;

void init();

void video_on();
void video_off();

typedef void (*Callback)();

void configure_timing(Timing const &, Callback = 0);

void configure_band(unsigned start, unsigned length, Rasterizer *);

void wait_for_vblank();

bool in_vblank();

void sync_to_vblank();

}  // namespace vga

#endif  // VGA_VGA_H
