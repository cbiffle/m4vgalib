#ifndef VGA_RAST_DIRECT_2_MIRROR_H
#define VGA_RAST_DIRECT_2_MIRROR_H

#include "vga/rasterizer.h"
#include "vga/rast/direct_2.h"

namespace vga {
namespace rast {

/*
 * A variation on Direct_2 that just redraws another rasterizer's content, but
 * upside down.
 */
class Direct_2_Mirror : public Rasterizer {
public:
  Direct_2_Mirror(Direct_2 const & rast, unsigned top_line = 0);

  virtual LineShape rasterize(unsigned, Pixel *) override;

  unsigned get_width() const { return _r.get_width(); }
  unsigned get_height() const { return _r.get_height(); }
  unsigned char *get_fg_buffer() const { return _r.get_fg_buffer(); }
  unsigned char *get_bg_buffer() const { return _r.get_bg_buffer(); }

private:
  Direct_2 const & _r;
  unsigned _top_line;
};

}  // namespace rast
}  // namespace vga

#endif  // VGA_RAST_DIRECT_2_MIRROR_H
