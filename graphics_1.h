#ifndef VGA_GRAPHICS_1_H
#define VGA_GRAPHICS_1_H

namespace vga {

/*
 * Provides primitive graphics operations on a 1bpp raster framebuffer.
 */
class Graphics1 {
public:
  Graphics1(void *fb, unsigned width, unsigned height, unsigned stride_words);

  void set_pixel(unsigned x, unsigned y);
  void clear_pixel(unsigned x, unsigned y);

private:
  void *_fb;
  unsigned _width_px;
  unsigned _height_px;
  unsigned _stride_words;

  void set_pixel_assist(unsigned, unsigned);
  void clear_pixel_assist(unsigned, unsigned);
};

}  // namespace vga

#endif  // VGA_GRAPHICS_1_H
