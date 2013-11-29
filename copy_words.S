.syntax unified

#ifdef ARM_CORTEX_M4

@ High-throughput block transfer using the FPU register set as a 128-byte
@ buffer.
@
@ Using a conservative cycle estimation for a 924-byte transfer (which triggers
@ all the corner cases) gives 544 cycles / 924 bytes, or 0.588 cycles/byte.
@ This closely approaches the throughput of a single AHB-Lite link.
@
@ Arguments:
@  r0  source address
@  r1  destination address
@  r2  number of words to transfer.
.section .ramcode,"ax",%progbits
.balign 4
.global _Z10copy_wordsPKmPmm
.thumb_func
_Z10copy_wordsPKmPmm:
      @ Name our registers.
      src   .req r0
      dst   .req r1
      count .req r2

      @ The caller may have been using floating point.
      @ Save the callee-save portion of the register file.
      vpush {s16 - s31}                                           @ 17

      @ "Warm up" the transfer engine, which wants to operate in units of
      @ 128 bytes, by making smaller transfers until count a multiple of 128.

      @ Special-case the single word transfer; the macro below won't work.
      tst count, #1                                               @ 1
      ittt ne                                                     @ 0 (aligned)
      vldmne.32 src!, {s0}                                        @ 2
      vstmne.32 dst!, {s0}                                        @ 2
      subne count, #1                                             @ 1
      nop                                                         @ 1

      .macro XFER n                             @ 6 + 2*n
        @ Note that this will unalign the instruction stream.
        @ Follow with nop for deterministic timing.
        tst count, #(\n + 1)                    @ 1
        ittt ne                                 @ 0 (aligned)
        vldmne.32 src!, {s0 - s\n}              @ 1+1+n
        vstmne.32 dst!, {s0 - s\n}              @ 1+1+n
        subne count, #(\n + 1)                  @ 1
      .endm

      XFER 1                                                      @ 8
      nop                                                         @ 1
      XFER 3                                                      @ 12
      nop                                                         @ 1
      XFER 7                                                      @ 20
      nop                                                         @ 1
      XFER 15                                                     @ 36

      @ Handle the case where we've been asked to transfer <32 words.
      cbz count, 1f                                               @ 1 (n.t.)

      @ All warmed up, transfer in units of 128 bytes.
0:    subs count, #32                                             @ 1
      vldm.32 src!, {s0 - s31}                                    @ 33
      vstm.32 dst!, {s0 - s31}                                    @ 33
      bne 0b                                                      @ 1-3??

      @ Restore FPU state.
1:    vpop {s16 - s31}                                            @ 17
      bx lr                                                       @ 1-3??

#else
  #error copy_words is not available for your architecture.
#endif
