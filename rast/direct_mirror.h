#ifndef VGA_RAST_DIRECT_MIRROR_H
#define VGA_RAST_DIRECT_MIRROR_H

#include "vga/rasterizer.h"
#include "vga/rast/direct.h"

namespace vga {
namespace rast {

/*
 * A variation on Direct that just redraws another rasterizer's content, but
 * upside down, backwards, and (possibly) vertically shifted.
 *
 * If this sounds like a rare use case, that's because it is.
 */
class DirectMirror : public Rasterizer {
public:
  DirectMirror(Direct const & rast, unsigned top_line = 0);

  virtual RasterInfo rasterize(unsigned, Pixel *) override;

  unsigned get_width() const { return _r.get_width(); }
  unsigned get_height() const { return _r.get_height(); }
  unsigned char *get_fg_buffer() const { return _r.get_fg_buffer(); }
  unsigned char *get_bg_buffer() const { return _r.get_bg_buffer(); }

private:
  Direct const & _r;
  unsigned _top_line;
};

}  // namespace rast
}  // namespace vga

#endif  // VGA_RAST_DIRECT_MIRROR_H
