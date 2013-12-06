#include "vga/graphics_1.h"

namespace vga {

Graphics1::Graphics1(void *fb, unsigned w, unsigned h, unsigned s)
  : _fb(fb),
    _width_px(w),
    _height_px(h),
    _stride_words(s) {}

void Graphics1::set_pixel(unsigned x, unsigned y) {
  if (x >= _width_px || y >= _height_px) return;
  *bit_addr(x, y) = 1;
}

void Graphics1::clear_pixel(unsigned x, unsigned y) {
  if (x >= _width_px || y >= _height_px) return;
  *bit_addr(x, y) = 0;
}

unsigned *Graphics1::bit_addr(unsigned x, unsigned y) {
  unsigned offset = reinterpret_cast<unsigned>(_fb);
  unsigned bit_base = offset * 32 + 0x22000000;

  return reinterpret_cast<unsigned *>(bit_base) + y * _width_px + x;
}

static void swap(unsigned &a, unsigned &b) {
  unsigned t = a;
  a = b;
  b = t;
}

static int abs(int v) {
  return v < 0 ? -v : v;
}

__attribute__((section(".ramcode")))
void Graphics1::set_line(unsigned x1, unsigned y1, unsigned x2, unsigned y2) {
  int dx = abs(x2 - x1);
  int dy = abs(y2 - y1);

  if (x1 > x2) {
    swap(x1, x2);
    swap(y1, y2);
  }

  int sy = (y1 < y2) ? _width_px : -_width_px;

  int err = dx - dy;

  unsigned *out = bit_addr(x1, y1);
  unsigned * const final_out = bit_addr(x2, y2);

  while (out != final_out) {
    *out = 1;
    int e2 = err * 2;
    if (e2 > -dy) {
      err -= dy;
      ++out;
    }
    if (e2 < dx) {
      err += dx;
      out += sy;
    }
  }
}

__attribute__((section(".ramcode")))
void Graphics1::clear_line(unsigned x1, unsigned y1, unsigned x2, unsigned y2) {
  int dx = abs(x2 - x1);
  int dy = abs(y2 - y1);

  if (x1 > x2) {
    swap(x1, x2);
    swap(y1, y2);
  }

  int sy = (y1 < y2) ? _width_px : -_width_px;

  int err = dx - dy;

  unsigned *out = bit_addr(x1, y1);
  unsigned * const final_out = bit_addr(x2, y2);

  while (out != final_out) {
    *out = 0;
    int e2 = err * 2;
    if (e2 > -dy) {
      err -= dy;
      ++out;
    }
    if (e2 < dx) {
      err += dx;
      out += sy;
    }
  }
}

}  // namespace vga
