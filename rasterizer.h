#ifndef VGA_RASTERIZER_H
#define VGA_RASTERIZER_H

#include <cstdint>

namespace vga {

struct Timing;

/*
 * A producer of pixels, one scanline at a time.  Some rasterizers may
 * generate pixels procedurally; others may use a backing framebuffer or
 * other store.  Either way, the rasterizer is responsible for either containing
 * the resources it needs or allocating them from the arena.
 */
class Rasterizer {
public:
  typedef std::uint8_t Pixel;

  /*
   * Implementations of rasterize return RasterInfo describing how they
   * expect their output to be displayed.
   */
  struct RasterInfo {
    // Number of black pixels to left of line.  Negative offsets shift the
    // start of active video earlier than anticipated (closer to the hsync)
    // and may impinge on code running during hblank.
    int offset;

    // Number of valid pixels generated in render target.
    unsigned length;

    // Number of additional CPU cycles per pixel.  By default, each pixel takes
    // 4 CPU cycles.  This can be used to add more -- for example, adding 4 here
    // halves the horizontal resolution.
    unsigned stretch_cycles;

    // How many times to repeat this line of raster output, after the first.
    unsigned repeat_lines;
  };

  /*
   * Produces a single scanline of pixels into a scan buffer.  Returns the
   * number of pixels generated and an optional offset.
   *
   * The line_number is relative to the first displayed line in the current
   * mode, not necessarily the first line handled by this Rasterizer!
   */
  virtual RasterInfo rasterize(unsigned line_number, Pixel *raster_target) = 0;

protected:
  ~Rasterizer() = default;
};

}  // namespace vga

#endif  // VGA_RASTERIZER_H
