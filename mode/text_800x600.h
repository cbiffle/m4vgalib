#ifndef VGA_MODE_TEXT_800X600_H
#define VGA_MODE_TEXT_800X600_H

#include "vga/mode.h"
#include "vga/rast/text_10x16.h"

namespace vga {
namespace mode {

class Text_800x600 : public Mode {
public:
  virtual void activate();
  virtual void deactivate();
  virtual void rasterize(unsigned line_number, Pixel *target);
  virtual Timing const &get_timing() const;

  void clear_framebuffer(Pixel);
  void put_char(unsigned col, unsigned row, Pixel fore, Pixel back, char);

private:
  rast::Text_10x16 _rr;
};

}  // namespace mode
}  // namespace vga

#endif  // VGA_MODE_TEXT_800X600_H
