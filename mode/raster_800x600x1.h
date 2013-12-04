#ifndef VGA_MODE_RASTER_800X600X1_H
#define VGA_MODE_RASTER_800X600X1_H

#include "vga/mode.h"
#include "vga/graphics_1.h"
#include "vga/rast/bitmap_1.h"

namespace vga {
namespace mode {

class Raster_800x600x1 : public Mode {
public:
  Raster_800x600x1();

  virtual void activate();
  virtual void rasterize(unsigned line_number, Pixel *target);
  virtual Timing const &get_timing() const;

  Graphics1 make_bg_graphics() const;
  void flip();

  void set_fg_color(Pixel);
  void set_bg_color(Pixel);

private:
  rast::Bitmap_1 _rr;
};

}  // namespace mode
}  // namespace vga

#endif  // VGA_MODE_RASTER_800X600X1_H
