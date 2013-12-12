#include "vga/bitmap.h"

#include "lib/common/algorithm.h"

using common::min;

#define INVARIANT(c) while (!(c)) {}
#define PRE(c) while (!(c)) {}

namespace vga {

static constexpr unsigned maskbits(unsigned n) {
  return (1 << n) - 1;
}

/*
 * Specialization of bitrow for when both source and dest are word-aligned.
 * This reduces to N word transfers and a final bitfield insert.
 */
static void bitrow_aligned(unsigned const *source,
                           unsigned *dest,
                           unsigned bit_count) {
  // TODO: should totally use copy_words here.
  while (bit_count >= 32) {
    *dest++ = *source++;
    bit_count -= 32;
  }

  if (bit_count) {
    unsigned mask = (1 << bit_count) - 1;
    *dest = (*dest & ~mask) | (*source & mask);
  }
}

/*
 * Specialization of bitrow for when source and dest are unaligned the same way.
 */
static void bitrow_almost_aligned(unsigned const *source,
                                  unsigned *dest,
                                  unsigned start_bit,
                                  unsigned bit_count) {
  PRE(start_bit < 32);
  unsigned bits = common::min(32u - start_bit, bit_count);
  unsigned mask = maskbits(bits) << start_bit;
  *dest = (*dest & ~mask) | (*source & mask);
  
  bitrow_aligned(source + 1, dest + 1, bit_count - bits);
}

/*
 * Specialization of bitrow for when only dest is aligned.
 */
__attribute__((noinline))
static void bitrow_dest_aligned(unsigned const *source,
                                unsigned source_offset,
                                unsigned *dest,
                                unsigned bit_count) {
  PRE(source_offset < 32);

  while (bit_count >= 32) {
    *dest = (source[0] >> source_offset) | (source[1] << (32 - source_offset));
    ++source;
    ++dest;
    bit_count -= 32;
  }

  if (bit_count) {
    unsigned s = source[0] >> source_offset;
    if (bit_count > 32 - source_offset) {
      s |= source[1] << (32 - source_offset);
    }
    *dest = (*dest & ~maskbits(bit_count)) | (s & maskbits(bit_count));
  }
}

/*
 * Specialization of bitrow for when only source is aligned.
 */
static void bitrow_source_aligned(unsigned const *source,
                                  unsigned *dest,
                                  unsigned dest_offset,
                                  unsigned bit_count) {
  PRE(dest_offset < 32);

  while (bit_count >= 32) {
    unsigned s = *source++;
    *dest = (*dest & maskbits(dest_offset)) | (s << dest_offset);
    dest++;
    *dest = (*dest & ~maskbits(dest_offset)) | (s >> (32 - dest_offset));
    bit_count -= 32;
  }

  if (bit_count) {
    unsigned s = *source & maskbits(bit_count);
    dest[0] = (dest[0] & maskbits(dest_offset)) | (s << dest_offset);
    if (bit_count > (32 - dest_offset)) {
      dest[1] = (dest[1] & ~maskbits(dest_offset)) | (s >> (32 - dest_offset));
    }
  }
}

__attribute__((noinline))
static void bitrow(unsigned const *source,
                   unsigned source_offset,
                   unsigned *dest,
                   unsigned dest_offset,
                   unsigned bit_count) {
  if (source_offset == 0 && dest_offset == 0) {
    bitrow_aligned(source, dest, bit_count);
  } else if (source_offset == dest_offset) {
    bitrow_almost_aligned(source, dest, source_offset, bit_count);
  } else if (source_offset == 0) {
    bitrow_source_aligned(source, dest, dest_offset, bit_count);
  } else if (dest_offset == 0) {
    bitrow_dest_aligned(source, source_offset, dest, bit_count);
  } else {
    // We can align one of the two using a single transfer.  Which?
    if (source_offset > dest_offset) {
      unsigned bits_moved = 32 - source_offset;
      unsigned s = *source >> source_offset;
      *dest = (*dest & maskbits(dest_offset)) | (s << dest_offset);
      bitrow_source_aligned(source + 1,
                            dest, 
                            dest_offset + bits_moved,
                            bit_count - bits_moved);
    } else {  // dest_offset > source_offset
      unsigned bits_moved = 32 - dest_offset;
      unsigned s = *source >> source_offset;
      *dest = (*dest & maskbits(dest_offset)) | (s << dest_offset);
      bitrow_dest_aligned(source,
                          source_offset + bits_moved,
                          dest + 1,
                          bit_count - bits_moved);
    }
  }
}

void bitblt(Bitmap const &source, unsigned source_x, unsigned source_y,
            Bitmap const &dest, Bitmap::Block const &dest_blk) {
  PRE(source_x < source.width_px);
  PRE(source_y < source.height_px);

  PRE(source_x + dest_blk.width <= source.width_px);
  PRE(source_y + dest_blk.height <= source.height_px);

  PRE(dest_blk.x < dest.width_px);
  PRE(dest_blk.y < dest.height_px);
  PRE(dest_blk.x + dest_blk.width <= dest.width_px);
  PRE(dest_blk.y + dest_blk.height <= dest.height_px);

  // TODO: allow for negative stride.
  // TODO: enable overlap -- for now we reject it.
  {
    unsigned *source_start = source.word_addr(source_x, source_y);
    unsigned *source_end   = source.word_addr(source_x + dest_blk.width,
                                              source_y + dest_blk.height);

    unsigned *dest_start = dest.word_addr(dest_blk.x, dest_blk.y);
    unsigned *dest_end   = dest.word_addr(dest_blk.x + dest_blk.width,
                                          dest_blk.y + dest_blk.height);
    
    PRE(dest_end < source_start || source_end < dest_start);
  }

  for (unsigned y = 0; y < dest_blk.height; ++y) {
    bitrow(source.word_addr(source_x, source_y + y),
           source_x % 32,
           dest.word_addr(dest_blk.x, dest_blk.y + y),
           dest_blk.x % 32,
           dest_blk.width);
  }
}

}  // namespace vga
