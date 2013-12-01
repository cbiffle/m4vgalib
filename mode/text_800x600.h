#ifndef VGA_MODE_TEXT_800X600_H
#define VGA_MODE_TEXT_800X600_H

#include "vga/mode.h"

namespace vga {
namespace mode {

class Text_800x600 : public Mode {
public:
  virtual void activate();
  virtual void deactivate();
  virtual void rasterize(unsigned line_number, Pixel *target);
  virtual Timing const &get_timing() const;

  void clear_framebuffer(Pixel);
  void type_char_raw(Pixel fore, Pixel back, char);
  void type_char(Pixel fore, Pixel back, char);
  void type_chars(Pixel fore, Pixel back, char const *);
  void cursor_to(unsigned col, unsigned row);

private:
  unsigned *_framebuffer;
  unsigned char *_font;
  unsigned _insertion_pos;
};

}  // namespace mode
}  // namespace vga

#endif  // VGA_MODE_TEXT_800X600_H
