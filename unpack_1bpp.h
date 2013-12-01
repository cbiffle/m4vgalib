#ifndef VGA_UNPACK_1BPP_H
#define VGA_UNPACK_1BPP_H

namespace vga {

void unpack_1bpp_impl(void const *input_line,
                      unsigned char const *clut,
                      unsigned char *render_target,
                      unsigned bytes_in_input);

}  // namespace vga

#endif  // VGA_UNPACK_1BPP_H
