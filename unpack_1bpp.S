.syntax unified
.section .ramcode,"ax",%progbits

@ My version of binutils can't handle the instruction:
@   msr APSR_g, bits
@ So we'll have to write it in machine code instead.
@ Recall that 'bits' is r6.
.macro MSR_APSR_G_R6      @ Worst case 2 cycles on M4
  .hword 0xF386  @ That '6' is for r6
  .hword 0x8400
.endm


@ 1bpp pixel unpacker using two-color lookup table.
@
@ Arguments:
@  r0  start of input line containing 1bpp packed pixels.
@  r1  CLUT pointer to two bytes: the zero color and high color.
@  r2  output scan buffer.
@  r3  width of input line in bytes.
.global _ZN3vga16unpack_1bpp_implEPKvPKhPhj
.thumb_func
_ZN3vga16unpack_1bpp_implEPKvPKhPhj:
      @ Name the arguments...
      framebuffer .req r0
      clut        .req r1
      target      .req r2
      bytes       .req r3

      @ Name temporaries...
      vclut0      .req r4
      vclut1      .req r5
      bits        .req r6

      @ We unpack pixels in groups of four by copying them into the
      @ vector comparison result flags field (GE) of the PSR.
      @ Once loaded there, we can use the sel instruction to look up
      @ four pixels at once in the CLUT, once it's been suitably
      @ prepared.
      @
      @ Oddly, when moving bits into the GE field of the PSR, they
      @ don't come from the low order bits (as in the BFI instruction)
      @ -- they must already be waiting in bits 19:16.  So we use
      @ rotations to move pixels through these bits.
      @
      @ We save a single cycle and four bytes on the final iteration
      @ by suppressing the final rotate instruction.
      @
      @ Side note: the code below assumes that this macro leaves the
      @ condition flags unaffected.
      .macro UNPACK_FOUR_PIXELS rotate=1    @ 5 cyc (4 terminal)
        MSR_APSR_G_R6                 @ Load GE field.              2
        sel colors, vclut1, vclut0    @ Use it to mux colors.       1
        str colors, [target], #4      @ Write to raster target.     1
        .if \rotate
          ror bits, #4                @ Prepare next pixels.        1
        .endif
      .endm

      @ Actual code from here:

      push { vclut0, vclut1, bits }     @ Free up registers.                4

      @ Prepare vector CLUTs in callee-save registers.
      @ We could load these in one go, yes, but the math below requires the
      @ top 24 bits to be clear, and splitting the bytes and clearing the
      @ high bits takes several more cycles than a mere SRAM load.
      ldrb vclut0, [clut]               @ Zero color                        2
      ldrb vclut1, [clut, #1]           @ One color                         1

      @ Replicate each color's byte into all four byte lanes.
      @ Because the top 24 bits of each register are clear, we can do this
      @ with multiplication by a repeating bit pattern.  Both loading a
      @ repeating bit pattern and multiplication are very cheap on this
      @ architecture!
      mov clut, #0x01010101             @ Magic byte-lane smear constant.   1
      mul vclut0, vclut0, clut          @ Make vectors                      1
      mul vclut1, vclut1, clut          @ out of each entry.                1

      @ The register formerly known as clut changes roles here...
      colors .req clut
      .unreq clut

      @ Total cycles for startup:                                          10

      @ Enough paperwork.  Start unpacking!
0:    ldr bits, [framebuffer], #4       @ Load a block of 32 pixels.        2
      ror bits, #16                     @ Rotate bits 3:0 into 19:16.       1

      @ The unrolled 32-pixel unpacking code:
      UNPACK_FOUR_PIXELS    @ Pixels 0-3                                    5
      UNPACK_FOUR_PIXELS    @ Pixels 4-7                                    5
      UNPACK_FOUR_PIXELS    @ Pixels 8-11                                   5
      UNPACK_FOUR_PIXELS    @ Pixels 12-15                                  5
      UNPACK_FOUR_PIXELS    @ Pixels 16-19                                  5
      UNPACK_FOUR_PIXELS    @ Pixels 20-23                                  5
      UNPACK_FOUR_PIXELS    @ Pixels 24-27                                  5
      subs bytes, #4        @ Hoist to improve branch speculation below.    1
      UNPACK_FOUR_PIXELS 1  @ Pixels 28-31                                  4
      bhi 0b                                                              @ 3/1

      @ Total cycles for loop body:                                        46/44

      @ Aaaaaand we're done.
      pop { vclut0, vclut1, bits }                                        @ 4
      bx lr   @ TODO: is this any more efficient than storing lr / popping pc?
              @                                                             3?
      @ Cycles for 800-pixel line: 1165, or 1.4562 c/p (2.75x realtime)
      @ Empirically, it performs better than this, so I've doubtless been
      @ slightly too conservative about the processor's ability to speculate
      @ branches in the calculations above.
