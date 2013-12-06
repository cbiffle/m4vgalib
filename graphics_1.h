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

  void set_line(unsigned x1, unsigned y1, unsigned x2, unsigned y2);
  void clear_line(unsigned x1, unsigned y1, unsigned x2, unsigned y2);

private:
  void *_fb;
  unsigned _width_px;
  unsigned _height_px;
  unsigned _stride_words;

  unsigned *bit_addr(unsigned x, unsigned y);
};

}  // namespace vga

#endif  // VGA_GRAPHICS_1_H
