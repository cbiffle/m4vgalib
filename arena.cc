#include "vga/arena.h"

#include <cstdint>
#include <cstddef>

#include "etl/assert.h"
#include "etl/mem/arena.h"

using std::uint8_t;
using std::uintptr_t;
using std::size_t;

using etl::data::RangePtr;

namespace vga {

extern "C" {
  extern uint8_t _ccm_arena_start, _ccm_arena_end;
  extern uint8_t _sram112_arena_start, _sram112_arena_end;
}

using Arena = etl::mem::Arena<
  etl::mem::ReturnNullptrOnAllocationFailure,
  etl::mem::DoNotRequireDeallocation
>;

static Arena ccm_arena({&_ccm_arena_start, &_ccm_arena_end});
static Arena sram112_arena({&_sram112_arena_start, &_sram112_arena_end});

/*
 * This serves as a *prioritized* search list, so allocations will be made from
 * the first arena that fits.  We prioritize CCM over the SRAM112 bank because
 * it's smaller, so we exhaust it preferentially to leave room for large
 * buffers.
 */
static constexpr Arena * arenas[] = { &ccm_arena, &sram112_arena };

void arena_reset() {
  for (auto a : arenas) a->reset();
}

size_t arena_bytes_free() {
  size_t total = 0;
  for (auto a : arenas) total += a->get_free_count();
  return total;
}

size_t arena_bytes_total() {
  size_t total = 0;
  for (auto a : arenas) total += a->get_total_count();
  return total;
}

void * arena_alloc(size_t bytes) {
  for (auto a : arenas) {
    auto p = a->allocate(bytes);
    if (p) return p;
  }
  ETL_ASSERT(false);
}

}  // namespace vga
