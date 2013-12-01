depth := ..
products := vga

vga[type] := library

vga[cc_files] := vga.cc

vga[S_files] := \
  copy_words.S \
  unpack_1bpp.S

vga[cc_flags] := -std=gnu++0x

vga[libs] := \
  lib/stm32f4xx:stm32f4xx

include $(depth)/build/Makefile.rules
