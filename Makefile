depth := ..
products := vga

vga[type] := library

vga[cc_files] := \
  arena.cc \
  bitmap.cc \
  font_10x16.cc \
  graphics_1.cc \
  measurement.cc \
  rasterizer.cc \
  timing.cc \
  vga.cc \
  rast/bitmap_1.cc \
  rast/direct_4.cc \
  rast/solid_color.cc \
  rast/text_10x16.cc

vga[S_files] := \
  copy_words.S \
  graphics_1_assist.S \
  rast/unpack_1bpp.S \
  rast/unpack_direct_x4.S \
  rast/unpack_text_10p_attributed.S

vga[cc_flags] := -std=gnu++0x -O2

vga[libs] := \
  etl/common:common \
  lib/stm32f4xx:stm32f4xx

include $(depth)/build/Makefile.rules
