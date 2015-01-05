.syntax unified
.section .ramcode,"ax",%progbits

@ Palettized color unpacker with 4x linear interpolation atop 4x dithering,
@ for an overall 16x horizontal magnification factor.
@
@ Arguments:
@  r0  start of input line containing field values (palette indices).
@  r1  output scan buffer.
@  r2  width of input line in bytes.
@  r3  address of 256-byte palette.
.global _ZN3vga4rast25unpack_p256_lerp4_d4_implEPKvPhjPKhS5_
.thumb_func
_ZN3vga4rast25unpack_p256_lerp4_d4_implEPKvPhjPKhS5_:
      @ Name the arguments...
      input       .req r0
      output      .req r1
      input_end   .req r2
      palette0    .req r3

      @ Name the temporaries...
      left        .req r4
      right       .req r5
      delta       .req r6
      px1         .req r7
      px2         .req r8
      t1          .req r9
      t2          .req r10
      t3          .req r11
      palette1    .req r12

      @ Convert count to end-address.
      adds input_end, input

      @ Free temporaries.  palette1 is r12 and doesn't need saving.
      push {left, right, delta, px1, px2, t1, t2, t3}

      @ Load second palette.
      ldr palette1, [sp, #32]

      @ Materialize magical fixed-point ratio constants!
      @ Note that these are all 1/2 their target values, to avoid setting the
      @ sign bit.
      mov t1, #0x20000000
      mov t2, #0x40000000
      mov t3, #0x60000000

      @ Start the sliding window
      ldrb left, [input], #1

      @ Load endpoint for interpolation and find delta.
0:    ldrb right, [input], #1             @ 2
      subs delta, right, left             @ 1

      @ Double the delta to account for our limited fixed point range.
      adds delta, delta

      ldrb px1, [palette0, left]          @ 2
      ldrb px2, [palette1, left]          @ 1
      strb px1, [output], #1              @ 1
      strb px2, [output], #1              @ 1
      strb px1, [output], #1              @ 1
      strb px2, [output], #1              @ 1

      smmlar px2, delta, t1, left         @ 1
      ldrb px1, [palette0, px2]           @ 2
      ldrb px2, [palette1, px2]           @ 1
      strb px1, [output], #1              @ 1
      strb px2, [output], #1              @ 1
      strb px1, [output], #1              @ 1
      strb px2, [output], #1              @ 1

      smmlar px2, delta, t2, left         @ 1
      ldrb px1, [palette0, px2]           @ 2
      ldrb px2, [palette1, px2]           @ 1
      strb px1, [output], #1              @ 1
      strb px2, [output], #1              @ 1
      strb px1, [output], #1              @ 1
      strb px2, [output], #1              @ 1

      smmlar px2, delta, t3, left         @ 1
      ldrb px1, [palette0, px2]           @ 2
      ldrb px2, [palette1, px2]           @ 1
      strb px1, [output], #1              @ 1
      strb px2, [output], #1              @ 1
      strb px1, [output], #1              @ 1
      strb px2, [output], #1              @ 1

      cmp input, input_end                @ 1

      @ Shift to the right.
      mov left, right                     @ 1

      bne 0b                              @ 1 - 3

      pop {left, right, delta, px1, px2, t1, t2, t3}
      bx lr
