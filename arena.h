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

/*
 * Equivalent of operator new; constructs a T from arena memory by forwarding
 * the given arguments to a constructor.
 *
 * Consider arena_make as an alternative -- it works with types with non-trivial
 * destructors.
 */
template <typename T, typename ... Args>
T * arena_new(Args && ... args) {
  static_assert(std::is_trivially_destructible<T>::value,
                "vga::arena_new is only safe to use with types that won't mind "
                "if their destructor never gets called.");

  void * raw = arena_alloc(sizeof(T));
  return new(raw) T(etl::forward<Args>(args)...);
}

/*
 * Trivial smart pointer used with arena_make.
 */
template <typename T>
class ArenaPtr {
public:
  ArenaPtr() : _ptr(nullptr) {}

  explicit ArenaPtr(T * ptr) : _ptr(ptr) {}

  ArenaPtr(ArenaPtr && other) : _ptr(other._ptr) {
    other._ptr = nullptr;
  }
  ArenaPtr(ArenaPtr const &) = delete;

  template <typename U>
  ArenaPtr(ArenaPtr<U> && other) : _ptr(other._ptr) {
    other._ptr = nullptr;
  }

  explicit operator bool() const {
    return bool(_ptr);
  }

  ArenaPtr & operator=(ArenaPtr && other) {
    clear();
    _ptr = other._ptr;
    other._ptr = nullptr;
    return *this;
  }

  ArenaPtr & operator=(ArenaPtr const &) = delete;

  ~ArenaPtr() {
    clear();
  }

  void clear() {
    if (_ptr) {
      _ptr->~T();
      _ptr = nullptr;
    }
  }

  T * operator->() const {
    return _ptr;
  }

  T & operator*() const {
    return *_ptr;
  }

private:
  T * _ptr;

  template <typename U>
  friend class ArenaPtr;
};

/*
 * Constructs a T from arena memory by forwarding the given arguments to a
 * constructor.  The result is returned in an ArenaPtr to ensure that its
 * destructor eventually gets called.
 */
template <typename T, typename ... Args>
ArenaPtr<T> arena_make(Args && ... args) {
  void * raw = arena_alloc(sizeof(T));
  return ArenaPtr<T>(new(raw) T(etl::forward<Args>(args)...));
}

template <typename T>
T * arena_new_array(std::size_t element_count) {
  static_assert(std::is_trivially_destructible<T>::value,
                "vga::arena_new_array is only safe to use with types that "
                "won't mind if their destructor never gets called.");

  auto ptr = static_cast<T *>(arena_alloc(sizeof(T) * element_count));
  for (std::size_t i = 0; i < element_count; ++i) {
    new(&ptr[i]) T;
  }
  return ptr;
}

}  // namespace vga

#endif  // VGA_ARENA_H
