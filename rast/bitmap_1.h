#ifndef VGA_RAST_BITMAP_1_H
#define VGA_RAST_BITMAP_1_H

#include <atomic>
#include <cstdint>

#include "vga/bitmap.h"
#include "vga/rasterizer.h"
#include "vga/graphics_1.h"

namespace vga {
namespace rast {

class Bitmap_1 : public Rasterizer {
public:
  /*
   * Creates a 1bpp bitmap rasterizer with the given width, height, and
   * optional offset.
   */
  Bitmap_1(unsigned width, unsigned height, unsigned top_line = 0);

  /*
   * Creates a 1bpp bitmap rasterizer with the given width, height, background
   * image, and optional offset.
   */
  Bitmap_1(unsigned width, unsigned height, Pixel const * background,
           unsigned top_line = 0);

  ~Bitmap_1();

  RasterInfo rasterize(unsigned, unsigned, Pixel *) override;

  Bitmap get_bg_bitmap() const;
  Graphics1 make_bg_graphics() const;

  /*
   * Flips the display pages at the next vblank.
   */
  void pend_flip();

  /*
   * Flips the display pages right fricking now.
   */
  void flip_now();

  void set_fg_color(Pixel);
  void set_bg_color(Pixel);

  std::uint32_t *get_fg_buffer() const { return _fb[_page1]; }
  std::uint32_t *get_bg_buffer() const { return _fb[!_page1]; }

  bool can_fg_use_bitband() const;
  bool can_bg_use_bitband() const;
  void copy_bg_to_fg() const;

private:
  unsigned _lines;
  unsigned _words_per_line;
  unsigned _top_line;
  bool _page1;
  std::atomic<bool> _flip_pended;
  Pixel _clut[2];
  std::uint32_t *_fb[2];
  Pixel const * _background;
};

}  // namespace rast
}  // namespace vga

#endif  // VGA_RAST_BITMAP_1_H
