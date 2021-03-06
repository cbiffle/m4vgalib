#include "vga/rast/direct_mirror.h"

#include "etl/prediction.h"

#include "vga/arena.h"
#include "vga/copy_words.h"
#include "vga/vga.h"
#include "vga/rast/unpack_direct_rev.h"

namespace vga {
namespace rast {

DirectMirror::DirectMirror(Direct const & rast,
                           unsigned top_line,
                           bool flip_horizontal)
  : _r(rast),
    _top_line(top_line),
    _flip_horizontal{flip_horizontal} {}

__attribute__((section(".ramcode")))
auto DirectMirror::rasterize(unsigned cycles_per_pixel,
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
  line_number = height - line_number - 1 + _flip_horizontal;

  auto const *src = get_fg_buffer() + width * line_number;

  if (_flip_horizontal) {
    unpack_direct_rev_impl(src, target, width);
  } else {
    copy_words(
        (uint32_t const *) (void const *) src,
        (uint32_t *) (void *) target,
        width / sizeof(uint32_t));
  }

  return {
    .offset = 0,
    .length = width,
    .cycles_per_pixel = scale_x * cycles_per_pixel,
    .repeat_lines = repeat,
  };
}

}  // namespace rast
}  // namespace vga
