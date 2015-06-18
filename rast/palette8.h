#ifndef VGA_RAST_PALETTE8_H
#define VGA_RAST_PALETTE8_H

#include <atomic>
#include <cstdint>

#include "vga/rasterizer.h"

namespace vga {
namespace rast {

/*
 * A palettized rasterizer that can multiply pixels on both axes -- which is
 * good, because it can't quite keep up with scanout at full resolution.
 *
 * This is deliberately designed to work like Direct.
 */
class Palette8 : public Rasterizer {
public:
  /*
   * An index into the palette.
   */
  using Index = std::uint8_t;

  /*
   * Creates a Palette8 with the given configuration:
   * - disp_width and disp_height give the native size of the display, e.g.
   *   800x600.
   * - scale_x and scale_y give the subdivision factors.  Both should be
   *   greater than zero.
   * - top_line applies an offset to the start of rasterization, for use when
   *   the rasterizer starts somewhere other than the top line of the display.
   */
  Palette8(unsigned disp_width, unsigned disp_height,
           unsigned scale_x, unsigned scale_y,
           unsigned top_line = 0);
  ~Palette8();

  RasterInfo rasterize(unsigned, unsigned, Pixel *) override;

  /*
   * Flips pages right now.  If video is active this will take effect at the
   * next line.
   */
  void flip_now();

  unsigned get_width() const { return _width; }
  unsigned get_height() const { return _height; }
  unsigned get_scale_x() const { return _scale_x; }
  unsigned get_scale_y() const { return _scale_y; }

  Index *get_fg_buffer() const { return _fb[_page1]; }
  Index *get_bg_buffer() const { return _fb[!_page1]; }

  Pixel * get_palette() { return _palette; }
  Pixel const * get_palette() const { return _palette; }

private:
  unsigned _width;
  unsigned _height;
  unsigned _scale_x;
  unsigned _scale_y;
  unsigned _top_line;
  Index *_fb[2];
  Pixel * _palette;
  bool _page1;
};

}  // namespace rast
}  // namespace vga

#endif  // VGA_RAST_PALETTE8_H
