#ifndef VGA_RAST_UNPACK_DIRECT_X2_REV_H
#define VGA_RAST_UNPACK_DIRECT_X2_REV_H

namespace vga {
namespace rast {

void unpack_direct_x2_rev_impl(void const *input_line,
                               unsigned char *render_target,
                               unsigned bytes_in_input);

}  // namespace rast
}  // namespace vga

#endif  // VGA_RAST_UNPACK_DIRECT_X2_REV_H
