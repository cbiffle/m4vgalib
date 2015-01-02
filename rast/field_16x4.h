#ifndef VGA_RAST_FIELD_16X4_H
#define VGA_RAST_FIELD_16X4_H

#include <atomic>

#include "vga/rasterizer.h"

namespace vga {
namespace rast {

/*
 * Rasterizes a subsampled scalar field using horizontal linear interpolation,
 * with optional positional dithering.
 *
 * This has the effect of magnifying the field 16x along the X axis and 4x along
 * the Y, so a 50x150 scalar field is drawn as an 800x600 image.
 */
class Field16x4 : public Rasterizer {
public:
  Field16x4(unsigned width, unsigned height, unsigned top_line = 0);
  ~Field16x4();

  virtual RasterInfo rasterize(unsigned, Pixel *) override;

  void pend_flip();
  void flip_now();

  unsigned get_width() const { return _width; }
  unsigned get_height() const { return _height; }
  unsigned char *get_fg_buffer() const { return _fb[_page1]; }
  unsigned char *get_bg_buffer() const { return _fb[!_page1]; }

  Pixel * get_palette(unsigned index) {
    return _palettes[index];
  }

private:
  unsigned _width;
  unsigned _height;
  unsigned _top_line;
  bool _page1;
  std::atomic<bool> _flip_pended;
  unsigned char *_fb[2];
  Pixel * _palettes[2];
};

}  // namespace rast
}  // namespace vga

#endif  // VGA_RAST_FIELD_16X4_H
