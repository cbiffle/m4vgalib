#ifndef VGA_GRAPHICS_1_H
#define VGA_GRAPHICS_1_H

namespace vga {

/*
 * Provides primitive graphics operations on a 1bpp raster framebuffer.
 *
 * Requires the framebuffer to be in bitband-capable memory, for performance.
 */
class Graphics1 {
public:
  Graphics1(void *fb, unsigned width, unsigned height, unsigned stride_words);

  void set_pixel(unsigned x, unsigned y);
  void clear_pixel(unsigned x, unsigned y);

  void draw_line(int x1, int y1, int x2, int y2, bool set);
  void draw_line(float x1, float y1, float x2, float y2, bool set);

  void set_line(int x1, int y1, int x2, int y2);
  void clear_line(int x1, int y1, int x2, int y2);

private:
  void *_fb;
  unsigned _width_px;
  unsigned _height_px;
  unsigned _stride_words;

  unsigned *bit_addr(unsigned x, unsigned y);

  unsigned compute_out_code(int x, int y);
  unsigned compute_out_code(float x, float y);

  void draw_line_clipped(int x1, int y1, int x2, int y2, bool set);
  void draw_line_clipped_y(unsigned *start, int dx, int dy, int dir, bool set);
  void draw_line_clipped_x(unsigned *start, int dx, int dy, int dir, bool set);
};

}  // namespace vga

#endif  // VGA_GRAPHICS_1_H
