#ifndef VGA_RAST_UNPACK_TEXT_10P_ATTRIBUTED_H
#define VGA_RAST_UNPACK_TEXT_10P_ATTRIBUTED_H

namespace vga {
namespace rast {

void unpack_text_10p_attributed_impl(void const *input_line,
                                     unsigned char const *font,
                                     unsigned char *render_target,
                                     unsigned cols_in_input);

}  // namespace rast
}  // namespace vga

#endif  // VGA_RAST_UNPACK_TEXT_10P_ATTRIBUTED_H
