#ifndef VGA_RAST_PALETTE8_4_H
#define VGA_RAST_PALETTE8_4_H

#include <atomic>

#include "vga/rasterizer.h"

namespace vga {
namespace rast {

/*
 * A palettized rasterizer that multiplies pixels by 4 on both axes, for
 * an overall 16x multiplication factor.  For example, this turns an 800x600
 * output timing mode into a 200x150 chunky graphics mode.
 */
class Palette8_4 : public Rasterizer {
public:
  Palette8_4(unsigned width, unsigned height, unsigned top_line = 0);
  ~Palette8_4();

  virtual LineShape rasterize(unsigned, Pixel *) override;

  void pend_flip();
  void flip_now();

  unsigned get_width() const { return _width; }
  unsigned get_height() const { return _height; }
  unsigned char *get_fg_buffer() const { return _fb[_page1]; }
  unsigned char *get_bg_buffer() const { return _fb[!_page1]; }

  Pixel * get_palette() { return _palette; }

private:
  unsigned _width;
  unsigned _height;
  unsigned _top_line;
  bool _page1;
  std::atomic<bool> _flip_pended;
  unsigned char *_fb[2];
  Pixel * _palette;
};

}  // namespace rast
}  // namespace vga

#endif  // VGA_RAST_PALETTE8_4_H
