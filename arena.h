#ifndef VGA_ARENA_H
#define VGA_ARENA_H

#include "etl/common/size.h"

namespace vga {

/*
 * Resets the arena state, implicitly destroying all objects allocated there.
 * Note that destructors are not called!
 */
void arena_reset();

etl::common::Size arena_bytes_free();
etl::common::Size arena_bytes_total();

}  // namespace vga

/*
 * Allocates a word-aligned chunk of memory from the arena.
 */
void *operator new(etl::common::Size);
void *operator new[](etl::common::Size);

/*
 * Does nothing.  The arena is deleted en-masse.
 */
void operator delete(void *);

#endif  // VGA_ARENA_H
