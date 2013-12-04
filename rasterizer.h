#ifndef VGA_RASTERIZER_H
#define VGA_RASTERIZER_H

namespace vga {

struct Timing;

/*
 * A producer of pixels, one scanline at a time.  Some rasterizers may
 * generate pixels procedurally; others may use a backing framebuffer or
 * other store.  Either way, the rasterizer is responsible for allocating
 * its own resources from the arena.
 */
class Rasterizer {
public:
  typedef unsigned char Pixel;

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
   * Allocate resources associated with this Rasterizer, typically from the
   * arena.  This is separated from construction so that Rasterizers can be
   * declared statically.
   *
   * The Rasterizer is shown the current timings in case it wants to adapt to
   * the screen resolution.
   *
   * The default implementation does nothing.
   */
  virtual void activate(Timing const &);

  /*
   * Produces a single scanline of pixels into a scan buffer.  Returns the
   * number of pixels generated and an optional offset.
   *
   * The line_number is relative to the first displayed line in the current
   * mode, not necessarily the first line handled by this Rasterizer!
   */
  virtual LineShape rasterize(unsigned line_number, Pixel *raster_target) = 0;

  /*
   * The opposite of activate.  In the usual case where rasterizer resources
   * come from the arena, this doesn't have anything to do except perhaps zero
   * some pointers.
   *
   * The default implementation does nothing.
   */
  virtual void deactivate();

  virtual ~Rasterizer();
};

}  // namespace vga

#endif  // VGA_RASTERIZER_H
