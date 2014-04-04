#ifndef VGA_RAST_DIRECT_4_H
#define VGA_RAST_DIRECT_4_H

#include "vga/rasterizer.h"

namespace vga {
namespace rast {

class Direct_4 : public Rasterizer {
public:
  Direct_4(unsigned width, unsigned height, unsigned top_line = 0);

  virtual void activate(Timing const &);
  virtual LineShape rasterize(unsigned, Pixel *);
  virtual void deactivate();

  void flip();
  void flip_now();

  unsigned char *get_fg_buffer() const { return _fb[_page1]; }
  unsigned char *get_bg_buffer() const { return _fb[!_page1]; }

private:
  unsigned char *_fb[2];
  unsigned _width;
  unsigned _height;
  unsigned _top_line;
  bool _page1;
};

}  // namespace rast
}  // namespace vga

#endif  // VGA_RAST_DIRECT_4_H
