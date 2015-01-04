#include "vga/rast/direct_2.h"

#include "etl/prediction.h"

#include "vga/arena.h"
#include "vga/copy_words.h"
#include "vga/vga.h"
#include "vga/rast/unpack_direct.h"

namespace vga {
namespace rast {

Direct_2::Direct_2(unsigned width, unsigned height, unsigned top_line)
  : _width(width),
    _height(height),
    _top_line(top_line),
    _page1(false),
    _fb{arena_new_array<unsigned char>(_width * _height),
        arena_new_array<unsigned char>(_width * _height)} {
  for (unsigned i = 0; i < _width * _height; ++i) {
    _fb[0][i] = 0;
    _fb[1][i] = 0;
  }
}

Direct_2::~Direct_2() {
  _fb[0] = _fb[1] = nullptr;
}

__attribute__((section(".ramcode")))
Rasterizer::RasterInfo Direct_2::rasterize(unsigned line_number, Pixel *target) {
  line_number -= _top_line;
  auto repeat = 1 - (line_number % 2);
  line_number /= 2;

  if (ETL_UNLIKELY(line_number >= _height)) return { 0, 0, 0, 0 };

  unsigned char const *src = _fb[_page1] + _width * line_number;

  unpack_direct_impl(src, target, _width);

  return { 0, _width, 4, repeat };
}

void Direct_2::flip_now() {
  _page1 = !_page1;
}

}  // namespace rast
}  // namespace vga
