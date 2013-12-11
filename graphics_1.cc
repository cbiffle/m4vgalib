#include "vga/graphics_1.h"

#define RAMCODE(sub) __attribute__((section(".ramcode." sub)))

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

RAMCODE("Graphics1.bit_addr")
unsigned *Graphics1::bit_addr(unsigned x, unsigned y) {
  unsigned offset = reinterpret_cast<unsigned>(_fb);
  unsigned bit_base = offset * 32 + 0x22000000;

  return reinterpret_cast<unsigned *>(bit_base) + y * _width_px + x;
}

static void swap(int &a, int &b) {
  int t = a;
  a = b;
  b = t;
}

enum {
  out_left = 1,
  out_right = 2,
  out_bottom = 4,
  out_top = 8,
};

RAMCODE("Graphics1.out_code.int")
unsigned Graphics1::compute_out_code(int x, int y) {
  return compute_out_code_spec(x, y);
}

RAMCODE("Graphics1.out_code.float")
unsigned Graphics1::compute_out_code(float x, float y) {
  return compute_out_code_spec(x, y);
}

template <typename T>
inline unsigned Graphics1::compute_out_code_spec(T x, T y) {
  unsigned code = 0;

  if (x < 0) code |= out_left;
  else if (x >= static_cast<int>(_width_px)) code |= out_right;

  if (y < 0) code |= out_top;
  else if (y >= static_cast<int>(_height_px)) code |= out_bottom;

  return code;
}

RAMCODE("Graphics1.draw_line.int")
void Graphics1::draw_line(int x1, int y1, int x2, int y2,
                          bool set) {
  draw_line_spec(x1, y1, x2, y2, set);
}

RAMCODE("Graphics1.draw_line.float")
void Graphics1::draw_line(float x1, float y1, float x2, float y2,
                          bool set) {
  draw_line_spec(x1, y1, x2, y2, set);
}

static inline int round_to_nearest(int x) { return x; }

static inline int round_to_nearest(float x) { return x + 0.5f; }

template <typename T>
inline void Graphics1::draw_line_spec(T x1, T y1, T x2, T y2, bool set) {
  unsigned code0 = compute_out_code(x1, y1);
  unsigned code1 = compute_out_code(x2, y2);

  while (code0 || code1) {
    if (code0 & code1) return;

    unsigned code = code0 ? code0 : code1;
    T x, y;

    if (code & out_bottom) {
      x = x1 + (x2 - x1) * (static_cast<int>(_height_px) - 1 - y1) / (y2 - y1);
      y = _height_px - 1;
    } else if (code & out_top) {
      x = x1 + (x2 - x1) * -y1 / (y2 - y1);
      y = 0;
    } else if (code & out_right) {
      y = y1 + (y2 - y1) * (static_cast<int>(_width_px) - 1 - x1) / (x2 - x1);
      x = _width_px - 1;
    } else /*if (code & out_left)*/ {
      y = y1 + (y2 - y1) * (-x1) / (x2 - x1);
      x = 0;
    }

    if (code == code0) {
      x1 = x;
      y1 = y;
      code0 = compute_out_code(x1, y1);
    } else {
      x2 = x;
      y2 = y;
      code1 = compute_out_code(x2, y2);
    }
  }

  draw_line_clipped(round_to_nearest(x1),
                    round_to_nearest(y1),
                    round_to_nearest(x2),
                    round_to_nearest(y2),
                    set);
}

RAMCODE("Graphics1.draw_line_clipped")
void Graphics1::draw_line_clipped_x(unsigned *out, int dx, int dy, int dir,
                                    bool set) {
  draw_line_clipped_spec<true>(out, dx, dy, dir, set);
}

RAMCODE("Graphics1.draw_line_clipped")
void Graphics1::draw_line_clipped_y(unsigned *out, int dx, int dy, int dir,
                                    bool set) {
  draw_line_clipped_spec<false>(out, dx, dy, dir, set);
}

template <bool H>
inline void Graphics1::draw_line_clipped_spec(unsigned *out, int dx, int dy,
                                              int dir, bool set) {
  int dmajor = H ? dx : dy;
  int dminor = H ? dy : dx;

  int minor_step = H ? _width_px : dir;
  int major_step = H ? dir : _width_px;

  int dminor2 = dminor * 2;
  int dmajor2 = dmajor * 2;
  int error = dminor2 - dmajor;

  *out = set;

  while (dmajor--) {
    if (error >= 0) {
      out += minor_step;
      error -= dmajor2;
    }
    error += dminor2;
    out += major_step;
    *out = set;
  }
}

RAMCODE("Graphics1.draw_line_clipped")
void Graphics1::draw_line_clipped(int x0, int y0, int x1, int y1, bool set) {
  if (y0 > y1) {
    swap(x0, x1);
    swap(y0, y1);
  }

  int dx = x1 - x0;
  int dy = y1 - y0;  // Nonnegative now

  unsigned *out = bit_addr(x0, y0);

  if (dx > 0) {
    if (dx > dy) {
      draw_line_clipped_x(out, dx, dy, 1, set);
    } else {
      draw_line_clipped_y(out, dx, dy, 1, set);
    }
  } else {
    dx = -dx;
    if (dx > dy) {
      draw_line_clipped_x(out, dx, dy, -1, set);
    } else {
      draw_line_clipped_y(out, dx, dy, -1, set);
    }
  }
}

RAMCODE("Graphics1.set_line.int")
void Graphics1::set_line(int x1, int y1, int x2, int y2) {
  draw_line(x1, y1, x2, y2, true);
}

RAMCODE("Graphics1.clear_line.int")
void Graphics1::clear_line(int x1, int y1, int x2, int y2) {
  draw_line(x1, y1, x2, y2, false);
}

RAMCODE("Graphics1.clear_all")
void Graphics1::clear_all() {
  unsigned *fb32 = static_cast<unsigned *>(_fb);
  unsigned *end = fb32 + _height_px * _stride_words;
  while (fb32 != end) *fb32++ = 0;
}

}  // namespace vga
