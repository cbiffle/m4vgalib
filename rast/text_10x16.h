#ifndef VGA_RAST_TEXT_10X16_H
#define VGA_RAST_TEXT_10X16_H

#include "vga/rasterizer.h"

namespace vga {
namespace rast {

class Text_10x16 : public Rasterizer {
public:
  Text_10x16(unsigned width, unsigned height, unsigned top_line = 0);

  virtual void activate(Timing const &);
  virtual LineShape rasterize(unsigned, Pixel *);
  virtual void deactivate();

  void clear_framebuffer(Pixel);

  void put_char(unsigned col, unsigned row,
                Pixel fore, Pixel back,
                char c);
  void put_packed(unsigned col, unsigned row, unsigned p);

  void set_x_adj(int v) { _x_adj = v; }

private:
  unsigned _cols;
  unsigned _rows;
  unsigned *_fb;
  unsigned char *_font;
  unsigned _top_line;
  int _x_adj;
};

}  // namespace rast
}  // namespace vga

#endif  // VGA_RAST_TEXT_10X16_H
