#ifndef VGA_ARENA_H
#define VGA_ARENA_H

#include "etl/size.h"

namespace vga {

/*
 * Resets the arena state, implicitly destroying all objects allocated there.
 * Note that destructors are not called!
 */
void arena_reset();

etl::Size arena_bytes_free();
etl::Size arena_bytes_total();

}  // namespace vga

/*
 * Allocates a word-aligned chunk of memory from the arena.
 */
void *operator new(etl::Size);
void *operator new[](etl::Size);

/*
 * Does nothing.  The arena is deleted en-masse.
 */
void operator delete(void *);

#endif  // VGA_ARENA_H
