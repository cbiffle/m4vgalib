#include "vga/rast/palette8_mirror.h"

#include "etl/prediction.h"

#include "vga/arena.h"
#include "vga/copy_words.h"
#include "vga/vga.h"
#include "vga/rast/unpack_p256.h"

namespace vga {
namespace rast {

Palette8Mirror::Palette8Mirror(Palette8 const & rast,
                               unsigned top_line)
  : _r(rast),
    _palette{arena_new_array<Pixel>(256)},
    _top_line(top_line) {
  for (unsigned i = 0; i < 256; ++i) {
    _palette[i] = 0;
  }
}

__attribute__((section(".ramcode")))
auto Palette8Mirror::rasterize(unsigned cycles_per_pixel,
                               unsigned line_number,
                               Pixel *target) -> RasterInfo {
  auto scale_x = _r.get_scale_x();
  auto scale_y = _r.get_scale_y();
  auto width = get_width();
  auto height = get_height();

  line_number -= _top_line;
  auto repeat = (scale_y - 1) - (line_number % scale_y);
  line_number /= scale_y;

  if (ETL_UNLIKELY(line_number >= height)) return { 0, 0, cycles_per_pixel, 0 };
  line_number = height - line_number - 1;

  auto const *src = get_fg_buffer() + width * line_number;

  unpack_p256_impl(src, target, width / sizeof(uint32_t), get_palette());

  return {
    .offset = 0,
    .length = width,
    .cycles_per_pixel = scale_x * cycles_per_pixel,
    .repeat_lines = repeat,
  };
}

}  // namespace rast
}  // namespace vga
