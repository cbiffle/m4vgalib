#include "vga/graphics_1.h"

#include "etl/utility.h"

#define RAMCODE(sub) __attribute__((section(".ramcode." sub)))

namespace vga {

/*******************************************************************************
 * Basics: construction, pixel access, memory geometry.
 */

Graphics1::Graphics1(Bitmap b)
  : _b(b) {}

void Graphics1::set_pixel(unsigned x, unsigned y) {
  if (x >= _b.width_px || y >= _b.height_px) return;
  *bit_addr(x, y) = 1;
}

void Graphics1::clear_pixel(unsigned x, unsigned y) {
  if (x >= _b.width_px || y >= _b.height_px) return;
  *bit_addr(x, y) = 0;
}

RAMCODE("Graphics1.word_addr")
unsigned *Graphics1::word_addr(unsigned x, unsigned y) {
  auto base32 = reinterpret_cast<unsigned *>(_b.base);
  return &base32[y * _b.stride_words + x/32];
}

RAMCODE("Graphics1.bit_addr")
unsigned *Graphics1::bit_addr(unsigned x, unsigned y) {
  unsigned offset = reinterpret_cast<unsigned>(_b.base);
  unsigned bit_base = offset * 32 + 0x22000000;

  return reinterpret_cast<unsigned *>(bit_base) + y * _b.width_px + x;
}

RAMCODE("Graphics1.clear_all")
void Graphics1::clear_all() {
  unsigned h = _b.height_px;
  unsigned w = _b.width_px;

  for (unsigned y = 0; y < h; ++y) {
    unsigned *row = word_addr(0, y);
    for (unsigned x = 0; x < w / 32; ++x) {
      row[x] = 0;
    }
  }
}


/*******************************************************************************
 * Bresenham-style line drawing.
 */

enum class Graphics1::Direction : bool {
  horizontal = true,
  vertical = false,
};

/*
 * The meat of the line drawing algorithm, specialized on three parameters:
 *
 * - S tells whether pixels should be set (true) or cleared (false).
 * - D tells whether the line is primarily horizontal (true) or vertical
 *   (false).  This gives the line's major axis.
 * - XAdv is either -1 or 1, and gives the direction of movement on the X
 *   axis (Y is always positive).
 */
template <bool S, Graphics1::Direction D, int XAdv>
inline void Graphics1::draw_line_unclipped_spec(unsigned *out,
                                                int dx, int dy) {
  int dmajor = D == Direction::horizontal ? dx : dy;
  int dminor = D == Direction::horizontal ? dy : dx;

  int minor_step = D == Direction::horizontal ? _b.width_px : XAdv;
  int major_step = D == Direction::horizontal ? XAdv : _b.width_px;

  int dminor2 = dminor * 2;
  int dmajor2 = dmajor * 2;
  int error = dminor2 - dmajor;

  *out = S;

  while (dmajor--) {
    if (error >= 0) {
      out += minor_step;
      error -= dmajor2;
    }
    error += dminor2;
    out += major_step;
    *out = S;
  }
}

template <bool S>
inline void Graphics1::draw_line_unclipped(int x0, int y0, int x1, int y1) {
  // We only draw horizontal or downward lines.  Ensure this:
  if (y0 > y1) {
    etl::swap(x0, x1);
    etl::swap(y0, y1);
  }

  int dx = x1 - x0;  // May be negative
  int dy = y1 - y0;  // Nonnegative now

  unsigned *out = bit_addr(x0, y0);

  if (dx == 0) {
    // Vertical line.
    // Note that dy is nonnegative.
    int step = _b.width_px;
    while (dy--) {
      *out = S;
      out += step;
    }
    return;
  }

  if (dy == 0) {
    // Horizontal line.
    int step = dx > 0 ? 1 : -1;
    while (dx -= step) {
      *out = S;
      out += step;
    }
    return;
  }

  if (dy == dx) {
    // Diagonal line to lower left.
    int step = _b.width_px + 1;
    while (dy--) {
      *out = S;
      out += step;
    }
    return;
  }

  if (dy == -dx) {
    // Diagonal line to lower right.
    int step = _b.width_px - 1;
    while (dy--) {
      *out = S;
      out += step;
    }
    return;
  }

  if (dx > 0) {  // Drawing to the left
    if (dx > dy) {  // Primarily horizontal (X is major axis)
      draw_line_unclipped_spec<S, Direction::horizontal, 1>(out, dx, dy);
    } else {  // Primarily vertical (Y is major axis)
      draw_line_unclipped_spec<S, Direction::vertical, 1>(out, dx, dy);
    }
  } else {  // Drawing to the right/straight up
    dx = -dx;  // dx is nonnegative now.
    if (dx > dy) {  // Primarily horizontal (X is major axis)
      draw_line_unclipped_spec<S, Direction::horizontal, -1>(out, dx, dy);
    } else {  // Primarily vertical (Y is major axis)
      draw_line_unclipped_spec<S, Direction::vertical, -1>(out, dx, dy);
    }
  }
}

/*
 * We only expose the unclipped version to users in this one flavor:
 */
void Graphics1::set_line_unclipped(int x1, int y1, int x2, int y2) {
  draw_line_unclipped<true>(x1, y1, x2, y2);
}


/*******************************************************************************
 * Line drawing with Cohen-Sutherland clipping.
 */

enum {
  out_left = 1,
  out_right = 2,
  out_bottom = 4,
  out_top = 8,
};

/*
 * Computes the "out code" for a given coordinate, telling which edges of the
 * screen (if any) a point is beyond.
 */
template <typename T>
inline unsigned Graphics1::compute_out_code_spec(T x, T y) {
  unsigned code = 0;

  if (x < 0) code |= out_left;
  else if (x >= int(_b.width_px)) code |= out_right;

  if (y < 0) code |= out_top;
  else if (y >= int(_b.height_px)) code |= out_bottom;

  return code;
}

/*
 * Explicit specialization of compute_out_code for int.
 */
RAMCODE("Graphics1.out_code.int")
unsigned Graphics1::compute_out_code(int x, int y) {
  return compute_out_code_spec(x, y);
}

/*
 * Explicit specialization of compute_out_code for float.
 */
RAMCODE("Graphics1.out_code.float")
unsigned Graphics1::compute_out_code(float x, float y) {
  return compute_out_code_spec(x, y);
}

/*
 * Utility function pair describing the conversion to int that we use at the
 * end of clipping.
 */
static inline int round_to_nearest(int x) { return x; }
static inline int round_to_nearest(float x) { return x + 0.5f; }

/*
 * The meat of the clipped line drawing algorithm.  This is specialized in terms
 * of two parameters:
 *
 * - S gives whether the pixels should be set (true) or cleared (false).
 * - T gives the coordinate type.
 */
template <bool S, typename T>
inline void Graphics1::draw_line_spec(T x1, T y1, T x2, T y2) {
  unsigned code0 = compute_out_code(x1, y1);
  unsigned code1 = compute_out_code(x2, y2);

  while (code0 || code1) {
    if (code0 & code1) return;  // Reject line; outside screen.

    unsigned code = code0 ? code0 : code1;
    T x, y;

    if (code & out_bottom) {
      x = x1 + (x2 - x1) * (int(_b.height_px) - 1 - y1) / (y2 - y1);
      y = _b.height_px - 1;
    } else if (code & out_top) {
      x = x1 + (x2 - x1) * -y1 / (y2 - y1);
      y = 0;
    } else if (code & out_right) {
      y = y1 + (y2 - y1) * (int(_b.width_px) - 1 - x1) / (x2 - x1);
      x = _b.width_px - 1;
    } else /*if (code & out_left)*/ {
      y = y1 + (y2 - y1) * -x1 / (x2 - x1);
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

  // Call through to the post-clipping draw routine specialization.
  draw_line_unclipped<S>(round_to_nearest(x1),
                       round_to_nearest(y1),
                       round_to_nearest(x2),
                       round_to_nearest(y2));
}

/*
 * Explicit specialization of draw_line_spec for setting pixels using int
 * coordinates.
 */
RAMCODE("Graphics1.set_line.int")
void Graphics1::set_line(int x1, int y1, int x2, int y2) {
  draw_line_spec<true>(x1, y1, x2, y2);
}

/*
 * Explicit specialization of draw_line_spec for clearing pixels using int
 * coordinates.
 */
RAMCODE("Graphics1.clear_line.int")
void Graphics1::clear_line(int x1, int y1, int x2, int y2) {
  draw_line_spec<false>(x1, y1, x2, y2);
}

/*******************************************************************************
 * Floating point coordinate support.
 */

RAMCODE("Graphics1.set_line.float")
void Graphics1::set_line(float x1, float y1, float x2, float y2) {
  draw_line_spec<true>(x1, y1, x2, y2);
}

RAMCODE("Graphics1.clear_line.float")
void Graphics1::clear_line(float x1, float y1, float x2, float y2) {
  draw_line_spec<false>(x1, y1, x2, y2);
}

}  // namespace vga
