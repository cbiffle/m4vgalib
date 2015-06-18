#include "vga/rast/palette8.h"

#include "etl/prediction.h"

#include "vga/arena.h"
#include "vga/copy_words.h"
#include "vga/vga.h"
#include "vga/rast/unpack_p256.h"

namespace vga {
namespace rast {

Palette8::Palette8(unsigned disp_width, unsigned disp_height,
                   unsigned scale_x, unsigned scale_y,
                   unsigned top_line)
  : _width{disp_width / scale_x},
    _height{disp_height / scale_y},
    _scale_x{scale_x},
    _scale_y{scale_y},
    _top_line{top_line},
    _fb{arena_new_array<Index>(_width * _height),
        arena_new_array<Index>(_width * _height)},
    _palette{arena_new_array<Pixel>(256)},
    _page1{false} {

  for (unsigned i = 0; i < _width * _height; ++i) {
    _fb[0][i] = 0;
    _fb[1][i] = 0;
  }
  for (unsigned i = 0; i < 256; ++i) {
    _palette[i] = 0;
  }
}

Palette8::~Palette8() {
  _fb[0] = _fb[1] = nullptr;
  _palette = nullptr;
}

__attribute__((section(".ramcode")))
Rasterizer::RasterInfo Palette8::rasterize(unsigned cycles_per_pixel,
                                           unsigned line_number,
                                           Pixel *target) {
  line_number -= _top_line;
  auto repeat = (_scale_y - 1) - (line_number % _scale_y);
  line_number /= _scale_y;

  if (ETL_UNLIKELY(line_number >= _height)) {
    return { 0, 0, cycles_per_pixel, 0 };
  }

  unsigned char const *src = _fb[_page1] + _width * line_number;

  unpack_p256_impl(src, target, _width, _palette);
  return {
    .offset = 0,
    .length = _width,
    .cycles_per_pixel = cycles_per_pixel * _scale_x,
    .repeat_lines = repeat,
  };
}

void Palette8::flip_now() {
  _page1 = !_page1;
}

}  // namespace rast
}  // namespace vga
