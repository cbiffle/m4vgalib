.syntax unified
.section .fastcode,"ax",%progbits

@ Sets a pixel.
@ Inputs:
@  r0  Graphics1 pointer
@  r1  x coordinate
@  r2  y coordinate
.global _ZN3vga9Graphics116set_pixel_assistEjj
.thumb_func
_ZN3vga9Graphics116set_pixel_assistEjj:
      push {lr}
      ldr lr, [r0, #12]         @ Load stride.
      ldr r0, [r0, #0]          @ Load framebuffer.

      @ Multiply y by stride and add x/32.
      lsrs r3, r1, #5
      mla r3, lr, r2, r3

      @ Produce bitmask for pixel within word.
      and r1, r1, #31           @ Get bit index from X.
      movs r2, #1               @ Obtain a bit.
      lsls r1, r2, r1           @ Shift it into place.
      
      @ Read-or-write.
      ldr r2, [r0, r3, lsl #2]
      orrs r2, r1
      str r2, [r0, r3, lsl #2]

      pop {pc}

@ Clears a pixel.
@ Inputs:
@  r0  Graphics1 pointer
@  r1  x coordinate
@  r2  y coordinate
.global _ZN3vga9Graphics118clear_pixel_assistEjj
.thumb_func
_ZN3vga9Graphics118clear_pixel_assistEjj:
      push {lr}
      ldr lr, [r0, #12]         @ Load stride.
      ldr r0, [r0, #0]          @ Load framebuffer.

      @ Multiply y by stride and add x/32.
      lsrs r3, r1, #5
      mla r3, lr, r2, r3

      @ Produce bitmask for pixel within word.
      and r1, r1, #31           @ Get bit index from X.
      movs r2, #1               @ Obtain a bit.
      lsls r1, r2, r1           @ Shift it into place.
      
      @ Read-or-write.
      ldr r2, [r0, r3, lsl #2]
      bics r2, r1
      str r2, [r0, r3, lsl #2]

      pop {pc}
