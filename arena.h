#ifndef VGA_ARENA_H
#define VGA_ARENA_H

namespace vga {

/*
 * Resets the arena state, implicitly destroying all objects allocated there.
 * Note that destructors are not called!
 */
void arena_reset();

unsigned arena_bytes_free();
unsigned arena_bytes_total();

}  // namespace vga

/*
 * Allocates a word-aligned chunk of memory from the arena.
 */
void *operator new(unsigned);
void *operator new[](unsigned);

/*
 * Does nothing.  The arena is deleted en-masse.
 */
void operator delete(void *);

#endif  // VGA_ARENA_H
