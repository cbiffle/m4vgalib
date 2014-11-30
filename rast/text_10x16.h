#ifndef VGA_RAST_TEXT_10X16_H
#define VGA_RAST_TEXT_10X16_H

#include <cstdint>

#include "vga/rasterizer.h"

namespace vga {
namespace rast {

class Text_10x16 : public Rasterizer {
public:
  Text_10x16(unsigned width, unsigned height, unsigned top_line = 0);
  ~Text_10x16();

  virtual LineShape rasterize(unsigned, Pixel *) override;

  void clear_framebuffer(Pixel);

  void put_char(unsigned col, unsigned row,
                Pixel fore, Pixel back,
                char c);
  void put_packed(unsigned col, unsigned row, unsigned p);

  void set_x_adj(int v) { _x_adj = v; }

private:
  unsigned _cols;
  unsigned _rows;
  unsigned _top_line;
  int _x_adj;
  std::uint8_t * _font;
  std::uint32_t * _fb;
};

}  // namespace rast
}  // namespace vga

#endif  // VGA_RAST_TEXT_10X16_H
