depth := ..
products := vga

vga[type] := library

vga[cc_files] := \
  arena.cc \
  font_10x16.cc \
  mode.cc \
  vga.cc \
  mode/raster_640x480x1.cc \
  mode/raster_800x600x1.cc \
  mode/text_800x600.cc

vga[S_files] := \
  copy_words.S \
  unpack_1bpp.S \
  unpack_text_10p_attributed.S

vga[cc_flags] := -std=gnu++0x

vga[libs] := \
  lib/stm32f4xx:stm32f4xx

include $(depth)/build/Makefile.rules
