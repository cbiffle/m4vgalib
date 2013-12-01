#include "vga/graphics_1.h"

namespace vga {

Graphics1::Graphics1(void *fb, unsigned w, unsigned h, unsigned s)
  : _fb(fb),
    _width_px(w),
    _height_px(h),
    _stride_words(s) {}

void Graphics1::set_pixel(unsigned x, unsigned y) {
  if (x >= _width_px || y >= _height_px) return;
  set_pixel_assist(x, y);
}

void Graphics1::clear_pixel(unsigned x, unsigned y) {
  if (x >= _width_px || y >= _height_px) return;
  clear_pixel_assist(x, y);
}

}  // namespace vga
