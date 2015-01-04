.syntax unified
.section .ramcode,"ax",%progbits

@ Direct color rasterizer.
@
@ Arguments:
@  r0  start of input line containing pixels.
@  r1  output scan buffer.
@  r2  width of input line in pixels (bytes).
.global _ZN3vga4rast18unpack_direct_implEPKvPhj
.thumb_func
_ZN3vga4rast18unpack_direct_implEPKvPhj:
      @ Name the arguments...
      framebuffer .req r0
      target      .req r1
      bytes       .req r2

      @ Name some temporaries...
      pixels      .req r3

      @ Go!
0:    ldr pixels, [framebuffer], #4
      str pixels, [target], #4
      subs bytes, #4
      bhi 0b

      @ Return
      bx lr
