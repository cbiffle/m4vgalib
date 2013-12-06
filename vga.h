#ifndef VGA_VGA_H
#define VGA_VGA_H

namespace vga {

struct Timing;
class Rasterizer;

void init();

void video_on();
void video_off();

void configure_timing(Timing const &);

void configure_band(unsigned start, unsigned length, Rasterizer *);

void wait_for_vblank();

bool in_vblank();

void sync_to_vblank();

/*
 * Applications can implement this function to receive a callback during hblank.
 */
extern void hblank_interrupt();

}  // namespace vga

#endif  // VGA_VGA_H
