#include "vga/rast/direct.h"

#include "etl/prediction.h"

#include "vga/arena.h"
#include "vga/vga.h"
#include "vga/copy_words.h"

namespace vga {
namespace rast {

Direct::Direct(unsigned disp_width, unsigned disp_height,
               unsigned scale_x, unsigned scale_y,
               unsigned top_line)
  : _width(disp_width / scale_x),
    _height(disp_height / scale_y),
    _scale_x(scale_x),
    _scale_y(scale_y),
    _top_line(top_line),
    _fb{arena_new_array<Pixel>(_width * _height),
        arena_new_array<Pixel>(_width * _height)},
    _page1{false},
    _flip_pended{false} {
  for (unsigned i = 0; i < _width * _height; ++i) {
    _fb[0][i] = 0;
    _fb[1][i] = 0;
  }
}

Direct::~Direct() {
  _fb[0] = _fb[1] = nullptr;
}

__attribute__((section(".ramcode")))
auto Direct::rasterize(unsigned cycles_per_pixel,
                       unsigned line_number,
                       Pixel *target) -> RasterInfo {
  line_number -= _top_line;
  auto repeat = (_scale_y - 1) - (line_number % _scale_y);
  line_number /= _scale_y;

  if (ETL_UNLIKELY(line_number >= _height)) {
    return { 0, 0, cycles_per_pixel, 0 };
  }

  auto const *src = _fb[_page1] + _width * line_number;

  copy_words(
      (uint32_t const *) (void const *) src,
      (uint32_t *) (void *) target,
      _width / sizeof(uint32_t));

  return {
    .offset = 0,
    .length = _width,
    .cycles_per_pixel = cycles_per_pixel * _scale_x,
    .repeat_lines = repeat,
  };
}

void Direct::flip_now() {
  _page1 = !_page1;
}

void Direct::pend_flip() {
  _flip_pended = true;
}

}  // namespace rast
}  // namespace vga
