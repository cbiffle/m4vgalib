#ifndef VGA_MODE_H
#define VGA_MODE_H

namespace vga {

struct Timing;

/*
 * Implements a particular display mode.  This is a combination of pixel
 * rasterization strategy, framebuffer format/size, and any auxiliary
 * resources required.
 */
class Mode {
public:
  typedef unsigned char Pixel;

  /*
   * This virtual destructor is provided for completeness, but modes should
   * not in general expect their destructors to be called.  Instead, the entire
   * arena will be destroyed.
   */
  virtual ~Mode();

  /*
   * Allocate resources associated with the Mode, from the arena or elsewhere.
   * This will be called before rasterize or get_timing.
   */
  virtual void activate() = 0;

  virtual void rasterize(unsigned line_number,
                         Pixel *raster_target) = 0;

  virtual Timing const &get_timing() const = 0;

  /*
   * Release any non-arena resources associated with the mode.  This should
   * rarely be necessary, so the default implementation just returns.
   */
  virtual void deactivate();
};

}  // namespace vga

#endif  // VGA_MODE_H
