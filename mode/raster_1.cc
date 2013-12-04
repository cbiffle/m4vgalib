#include "vga/mode/raster_1.h"

#include "vga/timing.h"

namespace vga {
namespace mode {

Raster_1::Raster_1(unsigned width, unsigned height, Timing const &timing)
  : _rr(width, height),
    _timing(timing) {}

void Raster_1::activate() {
  _rr.activate(_timing);
}

__attribute__((section(".ramcode")))
void Raster_1::rasterize(unsigned line_number, Pixel *target) {
  // Adjust frame line to displayed line.
  line_number -= _timing.video_start_line;
  (void) _rr.rasterize(line_number, target);
}

__attribute__((section(".ramcode")))
Timing const &Raster_1::get_timing() const {
  return _timing;
}

}  // namespace mode
}  // namespace vga
