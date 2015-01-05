#ifndef VGA_RAST_UNPACK_P256_LERP4_H
#define VGA_RAST_UNPACK_P256_LERP4_H

#include <cstdint>

namespace vga {
namespace rast {

void unpack_p256_lerp4_impl(void const *input_line,
                            unsigned char *render_target,
                            unsigned bytes_in_input,
                            std::uint8_t const * palette0);

}  // namespace rast
}  // namespace vga

#endif  // VGA_RAST_UNPACK_P256_LERP4_H
