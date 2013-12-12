#ifndef VGA_BITMAP_H
#define VGA_BITMAP_H

namespace vga {

struct Bitmap {
  void *base;
  unsigned width_px;
  unsigned height_px;
  int stride_words;

  unsigned *word_addr(unsigned x, unsigned y) const {
    unsigned *b = static_cast<unsigned *>(base);
    return &b[y * stride_words + x/32];
  }

  struct Block {
    unsigned x;
    unsigned y;
    unsigned width;
    unsigned height;
  };

};

void bitblt(Bitmap const &source, unsigned source_x, unsigned source_y,
            Bitmap const &dest, Bitmap::Block const &dest_blk);

}  // namespace vga

#endif  // VGA_BITMAP_H
