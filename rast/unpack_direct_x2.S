.syntax unified
.section .ramcode,"ax",%progbits

@ Pixel-doubling direct color unpacker.
@
@ Arguments:
@  r0  start of input line containing pixels.
@  r1  output scan buffer.
@  r2  width of input line in pixels (bytes).
.global _ZN3vga4rast21unpack_direct_x2_implEPKvPhj
.thumb_func
_ZN3vga4rast21unpack_direct_x2_implEPKvPhj:
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
      @ We do this even though the top two lanes are discarded, because
      @ constants of the form #0xABABABAB are cheap to materialize in
      @ Thumb-2.
      movs magic, #0x01010101

.macro output_from_bit n
      uxtb pixel, pixels, ROR #\n
      mul pixel, magic
      strh pixel, [target], #2
.endm

      @ Go!
0:    ldr pixels, [framebuffer], #4
      
      output_from_bit 0
      output_from_bit 8
      output_from_bit 16
      output_from_bit 24

      subs bytes, #4
      bhi 0b

      @ Return
      pop {pixel, pc}
