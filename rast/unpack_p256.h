#ifndef VGA_RAST_UNPACK_P256_H
#define VGA_RAST_UNPACK_P256_H

#include <cstdint>

namespace vga {
namespace rast {

void unpack_p256_impl(void const *input_line,
                      unsigned char *render_target,
                      unsigned words_in_input,
                      std::uint8_t const * palette);

}  // namespace rast
}  // namespace vga

#endif  // VGA_RAST_UNPACK_P256_H
