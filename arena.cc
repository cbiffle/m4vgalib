#include "vga/arena.h"

#include "etl/common/types.h"

using etl::common::UInt8;
using etl::common::UIntPtr;
using etl::common::Size;

namespace vga {

struct Region {
  void *start;
  void *end;

  UIntPtr start_bits() const {
    return reinterpret_cast<UIntPtr>(start);
  }

  UIntPtr end_bits() const {
    return reinterpret_cast<UIntPtr>(end);
  }

  Size size_in_bytes() const {
    return static_cast<Size>(end_bits() - start_bits());
  }

  void *take_bytes(Size n) {
    void *old_start = start;
    start = static_cast<UInt8 *>(old_start) + n;
    return old_start;
  }
};

extern "C" {
  extern Region const _arena_regions_start, _arena_regions_end;
}

static void fail_if(bool condition) {
  while (condition);
}

static unsigned region_count;
static Region *state;

Size arena_total_bytes;

void arena_reset() {
  // Inspect the ROM table to figure out our RAM layout.
  region_count = &_arena_regions_end - &_arena_regions_start;

  // We're going to steal some memory from the first region for our bookkeeping.
  // Make sure that'll work...
  fail_if(region_count < 1);

  Region const *reg = &_arena_regions_start;
  fail_if(reg[0].size_in_bytes() < sizeof(Region) * region_count);

  // Great.  Copy the ROM table into RAM.
  state = static_cast<Region *>(reg[0].start);
  for (unsigned i = 0; i < region_count; ++i) {
    state[i] = reg[i];
    arena_total_bytes += state[i].size_in_bytes();
  }

  // Adjust the size of the first region to acknowledge our bookkeeping memory.
  state[0].start = static_cast<Region *>(state[0].start) + region_count;
}

Size arena_bytes_free() {
  Size free = 0;
  for (unsigned i = 0; i < region_count; ++i) {
    free += state[i].size_in_bytes();
  }
  return free;
}

Size arena_bytes_total() {
  return arena_total_bytes;
}

}  // namespace vga

void *operator new(Size bytes) {
  // We allocate in words, not bytes, so round up if required.
  bytes = (bytes + 3) & ~3;

  // Take from the first region that can satisfy.
  for (unsigned i = 0; i < vga::region_count; ++i) {
    if (vga::state[i].size_in_bytes() >= bytes) {
      return vga::state[i].take_bytes(bytes);
    }
  }

  // Allocation failed!
  while (1);
}

void *operator new[](Size bytes) {
  return ::operator new(bytes);
}

void operator delete(void *) {
}
