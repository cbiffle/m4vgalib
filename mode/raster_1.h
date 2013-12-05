#ifndef VGA_MODE_RASTER_1_H
#define VGA_MODE_RASTER_1_H

#include "vga/mode.h"
#include "vga/rast/bitmap_1.h"

namespace vga {
namespace mode {

class Raster_1 : public Mode {
public:
  Raster_1(unsigned width, unsigned height, Timing const &);

  virtual void activate();
  virtual Timing const &get_timing() const;

  rast::Bitmap_1 &get_rasterizer() { return _rr; }

private:
  rast::Bitmap_1 _rr;
  Timing const &_timing;
};



}  // namespace mode
}  // namespace vga

#endif  // VGA_MODE_RASTER_1_H
