.syntax unified
.section .ramcode,"ax",%progbits

@ Pixel-quadrupling palettized color unpacker.
@
@ Arguments:
@  r0  start of input line containing pixels.
@  r1  output scan buffer.
@  r2  width of input line in pixels (bytes).
@  r3  address of 256-byte palette.
.global _ZN3vga4rast19unpack_p256_x4_implEPKvPhjPKh
.thumb_func
_ZN3vga4rast19unpack_p256_x4_implEPKvPhjPKh:
      @ Name the arguments...
      framebuffer .req r0
      target      .req r1
      bytes       .req r2
      palette     .req r3

      @ Name some temporaries...
      px0         .req r4
      px1         .req r5
      magic       .req lr

      @ Free temporary
      push {px0, px1, magic}

      @ Manufacture a mask that replicates a byte into all lanes.
      movs magic, #0x01010101

      @ Go!
0:    ldrb px0, [framebuffer], #1         @ 2
      ldrb px1, [framebuffer], #1         @ 1
      ldrb px0, [palette, px0]            @ 1
      ldrb px1, [palette, px1]            @ 1
      mul px0, magic                      @ 1
      str px0, [target], #4               @ 1
      mul px1, magic                      @ 1
      str px1, [target], #4               @ 1
      subs bytes, #2                      @ 1
      bhi 0b                              @ 1-3

      @ Return
      pop {px0, px1, pc}
