#include "vga/graphics_1.h"

namespace vga {

Graphics1::Graphics1(void *fb, unsigned w, unsigned h, unsigned s)
  : _fb(fb),
    _width_px(w),
    _height_px(h),
    _stride_words(s) {}

}  // namespace vga
