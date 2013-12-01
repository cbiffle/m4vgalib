#ifndef VGA_MODE_TEXT_800X600_H
#define VGA_MODE_TEXT_800X600_H

#include "vga/mode.h"

namespace vga {
namespace mode {

class Text_800x600 : public Mode {
public:
  virtual void activate();
  virtual void rasterize(unsigned line_number, Pixel *target);
  virtual Timing const &get_timing() const;

private:
  unsigned *_framebuffer;
  unsigned char *_font;
};

}  // namespace mode
}  // namespace vga

#endif  // VGA_MODE_TEXT_800X600_H
