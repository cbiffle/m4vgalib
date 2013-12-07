#ifndef VGA_UNPACK_DIRECT_X4_H
#define VGA_UNPACK_DIRECT_X4_H

namespace vga {

void unpack_direct_x4_impl(void const *input_line,
                           unsigned char *render_target,
                           unsigned bytes_in_input);

}  // namespace vga

#endif  // VGA_UNPACK_DIRECT_X4_H
