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
   * Implementations of rasterize return a LineShape.  This lets them render
   * only a subsection of the scanline; the rest will be left black.
   */
  struct LineShape {
    // Number of black pixels to left of line.  Negative offsets shift the
    // start of active video earlier than anticipated (closer to the hsync)
    // and may impinge on code running during hblank.
    int offset;

    // Number of valid pixels generated in render target.
    unsigned length;
  };

  /*
   * Produces a single scanline of pixels into a scan buffer.  Returns the
   * number of pixels generated and an optional offset.
   *
   * The line_number is relative to the first displayed line in the current
   * mode, not necessarily the first line handled by this Rasterizer!
   */
  virtual LineShape rasterize(unsigned line_number, Pixel *raster_target) = 0;

protected:
  ~Rasterizer() = default;
};

}  // namespace vga

#endif  // VGA_RASTERIZER_H
