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

    // Number of AHB cycles per pixel.  The default value for this in the
    // current mode is given to the rasterizer; the rasterizer may alter it
    // in the result if desired.
    unsigned cycles_per_pixel;

    // How many times to repeat this line of raster output, after the first.
    unsigned repeat_lines;
  };

  /*
   * Produces a single scanline of pixels into a scan buffer.  Returns a
   * RasterInfo instance describing what was done.
   *
   * The line_number is relative to the first displayed line in the current
   * mode, not necessarily the first line handled by this Rasterizer!
   */
  virtual RasterInfo rasterize(unsigned cycles_per_pixel,
                               unsigned line_number,
                               Pixel *raster_target) = 0;

protected:
  ~Rasterizer() = default;
};

}  // namespace vga

#endif  // VGA_RASTERIZER_H
