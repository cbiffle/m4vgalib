#ifndef VGA_GRAPHICS_1_H
#define VGA_GRAPHICS_1_H

#include "vga/bitmap.h"

namespace vga {

/*
 * Provides primitive graphics operations on a 1bpp raster framebuffer.
 *
 * Requires the framebuffer to be in bitband-capable memory, for performance.
 */
class Graphics1 {
public:
  Graphics1(Bitmap);

  void set_pixel(unsigned x, unsigned y);
  void clear_pixel(unsigned x, unsigned y);

  void set_line(int x1, int y1, int x2, int y2);
  void set_line(float x1, float y1, float x2, float y2);

  void clear_line(int x1, int y1, int x2, int y2);
  void clear_line(float x1, float y1, float x2, float y2);

  void set_line_unclipped(int x1, int y1, int x2, int y2);

  void draw_line(int x1, int y1, int x2, int y2, bool set);
  void draw_line(float x1, float y1, float x2, float y2, bool set);

  void clear_all();

private:
  Bitmap _b;

  unsigned *bit_addr(unsigned x, unsigned y);
  unsigned *word_addr(unsigned x, unsigned y);

  unsigned compute_out_code(int x, int y);
  unsigned compute_out_code(float x, float y);
  template <typename T>
  inline unsigned compute_out_code_spec(T x, T y);

  template <bool S, typename T>
  inline void draw_line_spec(T x1, T y1, T x2, T y2);

  template <bool S>
  inline void draw_line_clipped(int x1, int y1, int x2, int y2);

  template <bool S, bool H>
  inline void draw_line_clipped_spec(unsigned *, int, int, int);
};

}  // namespace vga

#endif  // VGA_GRAPHICS_1_H
