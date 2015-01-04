.syntax unified
.section .ramcode,"ax",%progbits

@ Direct color rasterizer that happens to work backwards.
@
@ Arguments:
@  r0  off-the-end address of the input line.
@  r1  output scan buffer.
@  r2  width of input line in pixels (bytes).
.global _ZN3vga4rast22unpack_direct_rev_implEPKvPhj
.thumb_func
_ZN3vga4rast22unpack_direct_rev_implEPKvPhj:
      @ Name the arguments...
      framebuffer .req r0
      target      .req r1
      bytes       .req r2

      @ Name some temporaries...
      pixels      .req r3

      @ Go!
0:    ldr pixels, [framebuffer, #-4]!
      rev pixels, pixels
      str pixels, [target], #4
      subs bytes, #4
      bhi 0b

      @ Return
      bx lr
