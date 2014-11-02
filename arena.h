#ifndef VGA_ARENA_H
#define VGA_ARENA_H

#include <cstddef>

namespace vga {

/*
 * Resets the arena state, implicitly destroying all objects allocated there.
 * Note that destructors are not called!
 */
void arena_reset();

std::size_t arena_bytes_free();
std::size_t arena_bytes_total();

}  // namespace vga

#endif  // VGA_ARENA_H
