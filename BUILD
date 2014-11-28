c_library('vga',
  sources = [
    'arena.cc',
    'bitmap.cc',
    'copy_words.S',
    'font_10x16.cc',
    'graphics_1.cc',
    'measurement.cc',
    'rasterizer.cc',
    'timing.cc',
    'vga.cc',

    'rast/bitmap_1.cc',
    'rast/direct_2.cc',
    'rast/direct_4.cc',
    'rast/solid_color.cc',
    'rast/text_10x16.cc',
    'rast/unpack_1bpp.S',
    'rast/unpack_direct_x2.S',
    'rast/unpack_direct_x4.S',
    'rast/unpack_text_10p_attributed.S',
  ],
  local = {
    # TODO(cbiffle): is this helpful?
    'cxx_flags': [ '-O2' ],
  },
  deps = [
    '//etl',
    '//etl:assert_loop',
    '//etl/mem',
    '//etl/stm32f4xx:stm32f4xx',
  ],
)
