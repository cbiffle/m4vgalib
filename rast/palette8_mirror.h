#ifndef VGA_RAST_PALETTE_MIRROR_H
#define VGA_RAST_PALETTE_MIRROR_H

#include "vga/rasterizer.h"
#include "vga/rast/palette8.h"

namespace vga {
namespace rast {

/*
 * A variation on Palette8 that just redraws another rasterizer's content, but
 * upside down, and using a separate palette.  The output can also be flipped
 * horizontal (i.e. scanned out backwards) and vertically shifted.
 *
 * If you can't imagine how you'd use this... you are not imagining hard enough.
 */
class Palette8Mirror : public Rasterizer {
public:
  Palette8Mirror(Palette8 const & rast,
                 unsigned top_line);

  RasterInfo rasterize(unsigned, unsigned, Pixel *) override;

  unsigned get_width() const { return _r.get_width(); }
  unsigned get_height() const { return _r.get_height(); }
  unsigned char *get_fg_buffer() const { return _r.get_fg_buffer(); }
  unsigned char *get_bg_buffer() const { return _r.get_bg_buffer(); }

  Pixel * get_palette() { return _palette; }

private:
  Palette8 const & _r;
  Pixel * _palette;
  unsigned _top_line;
  bool _flip_horizontal;
};

}  // namespace rast
}  // namespace vga

#endif  // VGA_RAST_PALETTE_MIRROR_H
