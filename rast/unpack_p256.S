.syntax unified
.section .text

.balign 4
      nop.n

@ Palettized color unpacker.
@
@ Arguments:
@  r0  start of input line containing pixels.
@  r1  output scan buffer.
@  r2  width of input line in words.
@  r3  address of 256-byte palette.
.global _ZN3vga4rast16unpack_p256_implEPKvPhjPKh
.thumb_func
_ZN3vga4rast16unpack_p256_implEPKvPhjPKh:
      @ Name the arguments...
      framebuffer .req r0
      target      .req r1
      words       .req r2
      palette     .req r3

      @ Name some temporaries...
      px0         .req r4
      px1         .req r5
      px2         .req r6
      px3         .req r7

      @ Free temporary
      push {px0, px1, px2, px3, lr}

      @ Go!
0:    ldrb px3, [framebuffer, #3]         @ 2
      ldrb px2, [framebuffer, #2]         @ 1
      ldrb px1, [framebuffer, #1]         @ 1
      ldrb px0, [framebuffer], #4         @ 1
      ldrb px3, [palette, px3]            @ 1
      ldrb px2, [palette, px2]            @ 1
      ldrb px1, [palette, px1]            @ 1
      ldrb px0, [palette, px0]            @ 1
      strb px3, [target, #3]              @ 1
      strb px2, [target, #2]              @ 1
      strb px1, [target, #1]              @ 1
      strb px0, [target], #4              @ 1
      subs words, #1                      @ 1
      bhi 0b                              @ 1-3

      @ Return
      pop {px0, px1, px2, px3, pc}
