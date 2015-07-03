#include "vga/rast/field_16x4.h"

#include "etl/prediction.h"

#include "vga/arena.h"
#include "vga/copy_words.h"
#include "vga/vga.h"
#include "vga/rast/unpack_p256_lerp4_d4.h"

namespace vga {
namespace rast {

Field16x4::Field16x4(unsigned width, unsigned height, unsigned top_line)
  : _width(width),
    _height(height),
    _top_line(top_line),
    _page1(false),
    _flip_pended(false),
    _fb{arena_new_array<unsigned char>(_width * _height),
        arena_new_array<unsigned char>(_width * _height)},
    _palettes{
      arena_new_array<Pixel>(256),
      arena_new_array<Pixel>(256),
    } {
  for (unsigned i = 0; i < _width * _height; ++i) {
    _fb[0][i] = 0;
    _fb[1][i] = 0;
  }
  for (unsigned i = 0; i < 256; ++i) {
    _palettes[0][i] = 0;
    _palettes[1][i] = 0;
  }
}

Field16x4::~Field16x4() {
  _fb[0] = _fb[1] = nullptr;
  _palettes[0] = _palettes[1] = nullptr;
}

__attribute__((section(".ramcode")))
auto Field16x4::rasterize(unsigned cycles_per_pixel,
                          unsigned line_number,
                          Pixel *target) -> RasterInfo {
  line_number -= _top_line;
  auto repeat = 1 - (line_number % 2);
  line_number /= 2;
  bool odd_line = line_number & 1;
  line_number /= 2;

  if (ETL_UNLIKELY(line_number == 0)) {
    if (_flip_pended.exchange(false)) flip_now();
  }

  if (ETL_UNLIKELY(line_number >= _height)) {
    return { 0, 0, cycles_per_pixel, 0 };
  }

  unsigned char const *src = _fb[_page1] + _width * line_number;

  unpack_p256_lerp4_d4_impl(src, target, _width,
                            _palettes[odd_line], _palettes[!odd_line]);

  return {
    .offset = 0,
    .length = (_width - 1) * 16,
    .cycles_per_pixel = cycles_per_pixel,
    .repeat_lines = repeat,
  };
}

void Field16x4::pend_flip() {
  _flip_pended = true;
}

void Field16x4::flip_now() {
  _page1 = !_page1;
}

}  // namespace rast
}  // namespace vga
