.syntax unified
.section .ramcode,"ax",%progbits

@ Rasterizes 256-color text from a framebuffer containing per-character
@ attributes.
@
@ The character cells are 10 pixels wide, of which 8 pixels are read from a
@ font, and the remaining 2 provide inter-character spacing.
@
@ The input line is a sequence of 32-bit words, one per text column.  Each
@ word contains...
@  Bit
@   7: 0  8-bit character (font index).
@  15: 8  Background color.
@  23:16  Foreground color.
@  31:24  Attributes (currently unused).
@
@ Output pixels are generated using the font as a lookaside table.  Fonts are
@ stored row-normal: first the y=0 row of every glyph, then all the y=1 rows,
@ etc., packed 8 pixels to the byte.  This means to draw a row other than the
@ first, you pass an offset font pointer -- this unpacker has no idea how to
@ advance the row pointer.
@
@ You may have noticed that 10 is not a power of two.  To write the pixel output
@ most efficiently, we could handle pairs of characters, writing them out in
@ 5-word (20-pixel) groups.  This implementation does something simpler: it
@ ignores alignment concerns and takes the penalty cycles.  Throughput is still
@ adequate.
@
@ Otherwise, the implementation is very similar to the 1bpp unpacker, just with
@ a CLUT that changes every 10 pixels.

@ My version of binutils doesn't understand APSR_g.
.macro msr_APSR_g, rn
  .hword 0xF380 | \rn
  .hword 0x8400
.endm

@ Inputs:
@  r0  input line.
@  r1  font pointer.
@  r2  output raster target.
@  r3  width of line in text columns (4 bytes in input, 10 pixels in output).
@
.global _ZN3vga4rast31unpack_text_10p_attributed_implEPKvPKhPhj
.thumb_func
_ZN3vga4rast31unpack_text_10p_attributed_implEPKvPKhPhj:
      @ Name the inputs
      text    .req r0
      font    .req r1
      target  .req r2
      cols    .req r3

      @ Free up and name some working registers.  Note that we use LR as a
      @ temporary.
      fore    .req r4
      back    .req r5
      color0  .req r6
      bits    .req r7
      lsbs    .req lr

      @ Because we can't use a register alias with this particular macro,
      @ provide shorthand.
      .macro msr_APSR_g_bits
        msr_APSR_g 7  @ bits == r7 (above)
      .endm

      push {fore, back, color0, bits, lsbs}

      @ This constant is used to smear colors across byte lanes, because
      @ ARMv7-M doesn't have vector shuffle operations.
      mov lsbs, #0x01010101

      @ Get on with it!
      .balign 4
0:    @ Load an attributed character into bits.
      ldr bits, [text], #4

      @ Extract colors and character into separate registers.
      @ "bits" will hold the character.
      uxtb fore, bits, ROR #16
      uxtb back, bits, ROR #8
      uxtb bits, bits

      @ Smear colors across byte lanes.
      mul fore, fore, lsbs
      mul back, back, lsbs

      @ Load a row of glyph data from the font.
      ldrb bits, [font, bits]

      @ Mux fore and back to produce combined colors for each glyph pixel.
      @ We use the same approach as the 1bpp unpacker: stuffing glyph bits
      @ into the GE field of the PSR and using the sel instruction.
      @ First, shift the glyph bits so the LSBs are in 19:16.
      @ The high-order bits are preserved in 23:20.
      lsls bits, #16
      msr_APSR_g_bits
      sel color0, fore, back

      @ Now do it again.
      lsrs bits, #4
      msr_APSR_g_bits
      sel bits, fore, back    @ bits now holds pixels

      @ Advance column; hoisted to help speculation of the branch below.
      subs cols, #1

      @ Store eight pixels.
      @ TODO: does stm have any alignment constraints?
      str color0, [target], #4
      str bits, [target], #4

      @ Store the two pixel gutter.
      strh back, [target], #2

      @ Aaaand repeat.
      bne 0b

      pop {fore, back, color0, bits, pc}
