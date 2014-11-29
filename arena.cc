#include "vga/arena.h"

#include <cstdint>
#include <cstddef>

#include "etl/assert.h"
#include "etl/mem/arena.h"

using std::uint8_t;
using std::uintptr_t;
using std::size_t;

using etl::data::RangePtr;
using etl::mem::Arena;
using etl::mem::Region;

namespace vga {

extern "C" {
  extern Region const _arena_regions_start, _arena_regions_end;
}

static Arena<> arena({&_arena_regions_start, &_arena_regions_end});

void arena_reset() {
  arena.reset();
}

size_t arena_bytes_free() {
  return arena.get_free_count();
}

size_t arena_bytes_total() {
  return arena.get_total_count();
}

void * arena_alloc(size_t bytes) {
  return arena.allocate(bytes);
}

}  // namespace vga
