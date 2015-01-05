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
  Graphics1() = default;
  Graphics1(Bitmap);

  /*
   * Clears all the pixels in the Bitmap to the background color.
   */
  void clear_all();

  /*
   * Sets a pixel to the foreground color if it is within the Bitmap's bounds.
   */
  void set_pixel(unsigned x, unsigned y);

  /*
   * Clears a pixel to the background color if it is within the Bitmap's bounds.
   */
  void clear_pixel(unsigned x, unsigned y);

  /*
   * Draws a line from (x1, y1) to (x2, y2), setting pixels along the line to
   * either the background (false) or foreground (true) color depending on the
   * value of set.  Any portion of the line outside the Bitmap's bounds is left
   * undrawn.
   *
   * This is simply a convenience wrapper around set_line and clear_line for
   * code that doesn't know statically which color it wants.  If used with a
   * constant for 'set' it will tend to compile into the right underlying
   * operation.
   */
  inline void draw_line(int x1, int y1, int x2, int y2, bool set) {
    if (set) {
      set_line(x1, y1, x2, y2);
    } else {
      clear_line(x1, y1, x2, y2);
    }
  }

  /*
   * Draws a line from (x1, y1) to (x2, y2), setting pixels along the line to
   * the foreground color.  Any portion of the line outside the Bitmap's bounds
   * is left undrawn.
   */
  void set_line(int x1, int y1, int x2, int y2);

  /*
   * Draws a line from (x1, y1) to (x2, y2), setting pixels along the line to
   * the background color.  Any portion of the line outside the Bitmap's bounds
   * is left undrawn.
   */
  void clear_line(int x1, int y1, int x2, int y2);

  /*
   * Special version of set_line that bypasses the Bitmap clipping logic, which
   * makes the operation very slightly faster -- and also dangerous.  If the
   * coordinates given fall outside the Bitmap, it will produce out-of-bounds
   * memory writes.  Use this with caution and only when necessary.
   */
  void set_line_unclipped(int x1, int y1, int x2, int y2);

  /*
   * Float-coordinate alternate API.  These use floating point math in the
   * clipping routine, which can produce more accurate results, but at higher
   * cost.  In the end, the floats are reduced to integer pixel coordinates.
   */
  void set_line(float x1, float y1, float x2, float y2);
  void clear_line(float x1, float y1, float x2, float y2);

  inline void draw_line(float x1, float y1, float x2, float y2, bool set) {
    if (set) {
      set_line(x1, y1, x2, y2);
    } else {
      clear_line(x1, y1, x2, y2);
    }
  }

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
  inline void draw_line_unclipped(int x1, int y1, int x2, int y2);

  enum class Direction : bool;

  template <bool, Direction, int>
  inline void draw_line_unclipped_spec(unsigned *, int, int);
};

}  // namespace vga

#endif  // VGA_GRAPHICS_1_H
