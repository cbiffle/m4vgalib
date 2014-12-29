#ifndef VGA_RAST_UNPACK_1BPP_H
#define VGA_RAST_UNPACK_1BPP_H

#include <cstdint>

namespace vga {
namespace rast {

void unpack_1bpp_impl(std::uint32_t const *input_line,
                      std::uint8_t const *clut,
                      std::uint8_t *render_target,
                      unsigned words_in_input);

void unpack_1bpp_overlay_impl(std::uint32_t const *input_line,
                              std::uint8_t const *clut,
                              std::uint8_t *render_target,
                              unsigned words_in_input,
                              std::uint8_t const * background);

}  // namespace rast
}  // namespace vga

#endif  // VGA_RAST_UNPACK_1BPP_H
