#ifndef VGA_RAST_UNPACK_1BPP_H
#define VGA_RAST_UNPACK_1BPP_H

#include "etl/types.h"

namespace vga {
namespace rast {

void unpack_1bpp_impl(etl::UInt32 const *input_line,
                      etl::UInt8 const *clut,
                      etl::UInt8 *render_target,
                      unsigned words_in_input);

}  // namespace rast
}  // namespace vga

#endif  // VGA_RAST_UNPACK_1BPP_H
