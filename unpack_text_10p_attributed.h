#ifndef VGA_UNPACK_TEXT_10P_ATTRIBUTED_H
#define VGA_UNPACK_TEXT_10P_ATTRIBUTED_H

namespace vga {

void unpack_text_10p_attributed_impl(void const *input_line,
                                     unsigned char const *font,
                                     unsigned char *render_target,
                                     unsigned cols_in_input);

}  // namespace vga

#endif  // VGA_UNPACK_TEXT_10P_ATTRIBUTED_H
