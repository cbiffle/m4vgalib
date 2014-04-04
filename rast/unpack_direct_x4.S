.syntax unified
.section .ramcode,"ax",%progbits

@ Pixel-quadrupling direct color unpacker.
@
@ Arguments:
@  r0  start of input line containing pixels.
@  r1  output scan buffer.
@  r2  width of input line in pixels (bytes).
.global _ZN3vga4rast21unpack_direct_x4_implEPKvPhj
.thumb_func
_ZN3vga4rast21unpack_direct_x4_implEPKvPhj:
      @ Name the arguments...
      framebuffer .req r0
      target      .req r1
      bytes       .req r2

      @ Name some temporaries...
      pixels      .req r3
      pixel       .req r4
      magic       .req lr

      @ Free temporary
      push {pixel, magic}

      @ Manufacture a mask that replicates a byte into all lanes.
      movs magic, #0x01010101

      @ Go!
0:    ldr pixels, [framebuffer], #4
      
      uxtb pixel, pixels
      mul pixel, magic
      str pixel, [target], #4

      uxtb pixel, pixels, ROR #8
      mul pixel, magic
      str pixel, [target], #4

      uxtb pixel, pixels, ROR #16
      mul pixel, magic
      str pixel, [target], #4

      uxtb pixel, pixels, ROR #24
      mul pixel, magic
      str pixel, [target], #4

      subs bytes, #4
      bhi 0b

      @ Return
      pop {pixel, pc}
