#ifndef VGA_MODE_RASTER_640X480X1_H
#define VGA_MODE_RASTER_640X480X1_H

#include "vga/mode.h"
#include "vga/rast/bitmap_1.h"

namespace vga {
namespace mode {

class Raster_640x480x1 : public Mode {
public:
  Raster_640x480x1();

  virtual void activate();
  virtual void rasterize(unsigned line_number, Pixel *target);
  virtual Timing const &get_timing() const;

  rast::Bitmap_1 &get_rasterizer() { return _rr; }

private:
  rast::Bitmap_1 _rr;
};

}  // namespace mode
}  // namespace vga

#endif  // VGA_MODE_RASTER_640X480X1_H
