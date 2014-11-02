#ifndef VGA_ARENA_H
#define VGA_ARENA_H

#include <cstddef>
#include <new>
#include <type_traits>

#include "etl/utility.h"

namespace vga {

/*
 * Resets the arena state, implicitly destroying all objects allocated there.
 * Note that destructors are not called!
 */
void arena_reset();

std::size_t arena_bytes_free();
std::size_t arena_bytes_total();

/*
 * Fundamental allocation primitive; you probably don't want to use this
 * directly when the templates below are available....
 */
void * arena_alloc(std::size_t);

template <typename T, typename ... Args>
T * arena_new(Args && ... args) {
  static_assert(std::is_trivially_destructible<T>::value,
    "The arena allocator is only safe to use with types that won't mind "
    "if their destructor never gets called.");

  void * raw = arena_alloc(sizeof(T));
  return new(raw) T(etl::forward<Args>(args)...);
}

template <typename T>
T * arena_new_array(std::size_t element_count) {
  static_assert(std::is_trivially_destructible<T>::value,
    "The arena allocator is only safe to use with types that won't mind "
    "if their destructor never gets called.");

  auto ptr = static_cast<T *>(arena_alloc(sizeof(T) * element_count));
  for (std::size_t i = 0; i < element_count; ++i) {
    new(&ptr[i]) T;
  }
  return ptr;
}

}  // namespace vga

#endif  // VGA_ARENA_H
