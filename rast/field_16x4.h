#ifndef VGA_RAST_FIELD_16X4_H
#define VGA_RAST_FIELD_16X4_H

#include <atomic>

#include "vga/rasterizer.h"

namespace vga {
namespace rast {

/*
 * Draws an 8-bit scalar field, upscaling by 16x horizontally using linear
 * interpolation.
 *
 * Scanlines are upscaled by 4x vertically.  Applications can perform their
 * own vertical linear interpolation by 4x to match the 16x horizontal factor.
 *
 * The field is rendered indirectly using a pair of palettes.  One palette is
 * used for the even pixels, and one for the odd.  The palettes change roles at
 * each scanline.  Configuring the palettes as slight shifts of one another
 * produces positional dithering; configuring them identically disables the
 * feature.
 *
 * Note that, due to interpolation, the field is *not* mod-256: value 255 is
 * 255 steps away from 0, not one step.
 */
class Field16x4 : public Rasterizer {
public:
  /*
   * Create a field rasterizer with the given number of points along each axis.
   *
   * Note that horizontal linear interpolation needs one extra column of data
   * just off the right edge of the screen.  The application may need to add
   * an extra row to support vertical interpolation as well, but the rasterizer
   * won't notice.
   *
   * The overall displayed size will be:
   * - disp_width  = (width - 1) * 16
   * - disp_height = height * 4
   */
  Field16x4(unsigned width, unsigned height, unsigned top_line = 0);
  ~Field16x4();

  RasterInfo rasterize(unsigned, unsigned, Pixel *) override;

  /*
   * Sets a flag requesting that the foreground and background fields swap
   * roles at the next start-of-active-video event.
   */
  void pend_flip();

  /*
   * Swaps the roles of the foreground and background fields.  If called during
   * active video, the field that was foreground before this call may still be
   * displayed until the end of the current scanline.
   *
   * This does not cancel any flip requested by pend_flip(); mixing the two is
   * odd.
   */
  void flip_now();

  /*
   * Returns the width (stride) of the scalar field, as provided to the
   * constructor.
   */
  unsigned get_width() const { return _width; }

  /*
   * Returns the height of the scalar field, as provided to the constructor.
   */
  unsigned get_height() const { return _height; }

  /*
   * Returns a pointer to the field currently being used for output.
   *
   * This is included for completeness and to support e.g. applications whose
   * next state depends on the contents of the previous field.  Modifying the
   * foreground buffer can produce unsightly artifacts.
   */
  uint8_t *get_fg_buffer() const { return _fb[_page1]; }

  /*
   * Returns a pointer to the field *not* currently being used for output.  It's
   * safe to alter this field until flip_now() or pend_flip() followed by
   * vblank.
   */
  uint8_t *get_bg_buffer() const { return _fb[!_page1]; }

  /*
   * Returns a pointer to the given palette (0 or 1).
   */
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
