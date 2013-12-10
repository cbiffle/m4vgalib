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

__attribute__((section(".ramcode")))
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

__attribute__((section(".ramcode")))
unsigned Graphics1::compute_out_code(int x, int y) {
  unsigned code = 0;

  if (x < 0) code |= out_left;
  else if (static_cast<unsigned>(x) >= _width_px) code |= out_right;

  if (y < 0) code |= out_top;
  else if (static_cast<unsigned>(y) >= _height_px) code |= out_bottom;

  return code;
}

__attribute__((section(".ramcode")))
unsigned Graphics1::compute_out_code(float x, float y) {
  unsigned code = 0;

  if (x < 0) code |= out_left;
  else if (x >= _width_px) code |= out_right;

  if (y < 0) code |= out_top;
  else if (y >= _height_px) code |= out_bottom;

  return code;
}


__attribute__((section(".ramcode")))
void Graphics1::draw_line(int x1, int y1, int x2, int y2,
                          bool set) {
  unsigned code0 = compute_out_code(x1, y1);
  unsigned code1 = compute_out_code(x2, y2);

  while (code0 || code1) {
    if (code0 & code1) return;

    unsigned code = code0 ? code0 : code1;
    int x, y;

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

  draw_line_clipped(x1, y1, x2, y2, set);
}

__attribute__((section(".ramcode")))
void Graphics1::draw_line(float x1, float y1, float x2, float y2,
                          bool set) {
  unsigned code0 = compute_out_code(x1, y1);
  unsigned code1 = compute_out_code(x2, y2);

  while (code0 || code1) {
    if (code0 & code1) return;

    unsigned code = code0 ? code0 : code1;
    float x, y;

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

  draw_line_clipped(x1 + 0.5f, y1 + 0.5f, x2 + 0.5f, y2 + 0.5f, set);
}

__attribute__((section(".ramcode")))
void Graphics1::draw_line_clipped_x(unsigned *out, int dx, int dy, int dir,
                                    bool set) {
  int dy2 = dy * 2;
  int dx2 = dx * 2;
  int error = dy2 - dx;

  unsigned stride = _width_px;

  *out = set;

  while (dx--) {
    if (error >= 0) {
      out += stride;
      error -= dx2;
    }
    error += dy2;
    out += dir;
    *out = set;
  }
}

__attribute__((section(".ramcode")))
void Graphics1::draw_line_clipped_y(unsigned *out, int dx, int dy, int dir,
                                    bool set) {
  int dx2 = dx * 2;
  int dy2 = dy * 2;
  int error = dx2 - dy;

  unsigned stride = _width_px;

  *out = set;

  while (dy--) {
    if (error >= 0) {
      out += dir;
      error -= dy2;
    }
    error += dx2;
    out += stride;
    *out = set;
  }
}

__attribute__((section(".ramcode")))
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

__attribute__((section(".ramcode")))
void Graphics1::set_line(int x1, int y1, int x2, int y2) {
  draw_line(x1, y1, x2, y2, true);
}

__attribute__((section(".ramcode")))
void Graphics1::clear_line(int x1, int y1, int x2, int y2) {
  draw_line(x1, y1, x2, y2, false);
}

}  // namespace vga
