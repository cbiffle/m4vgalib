#ifndef VGA_RAST_DIRECT_H
#define VGA_RAST_DIRECT_H

#include "vga/rasterizer.h"

namespace vga {
namespace rast {

/*
 * A direct-color rasterizer that multiplies pixels on both axes.  For example,
 * this can turn an 800x600 output timing mode into a 400x300 chunky graphics
 * mode.
 *
 * The factors on each axis are independent, in case 400x150 is more your speed.
 */
class Direct : public Rasterizer {
public:
  /*
   * Creates a Direct with the given configuration:
   * - disp_width and disp_height give the native size of the display, e.g.
   *   800x600.
   * - scale_x and scale_y give the subdivision factors.  Both should be
   *   greater than zero.
   * - top_line applies an offset to the start of rasterization, for use when
   *   this rasterizer starts somewhere other than the top of the display.
   */
  Direct(unsigned disp_width, unsigned disp_height,
         unsigned scale_x, unsigned scale_y,
         unsigned top_line = 0);
  ~Direct();

  RasterInfo rasterize(unsigned, unsigned, Pixel *) override;

  void flip_now();

  unsigned get_width() const { return _width; }
  unsigned get_height() const { return _height; }
  unsigned get_scale_x() const { return _scale_x; }
  unsigned get_scale_y() const { return _scale_y; }
  Pixel *get_fg_buffer() const { return _fb[_page1]; }
  Pixel *get_bg_buffer() const { return _fb[!_page1]; }

private:
  unsigned _width;
  unsigned _height;
  unsigned _scale_x;
  unsigned _scale_y;
  unsigned _top_line;
  bool _page1;
  Pixel *_fb[2];
};

}  // namespace rast
}  // namespace vga

#endif  // VGA_RAST_DIRECT_H
