#pragma once

#include <cstddef>
#include <new>
#include <type_traits>

namespace pluss::coden::detail {

template <class T, ::std::size_t Alignment> class AlignedAllocator {
  static_assert(Alignment >= alignof(T));
  static_assert((Alignment & (Alignment - 1)) == 0,
                "allocator alignment must be a power of two");

public:
  using value_type = T;
  using size_type = ::std::size_t;
  using difference_type = ::std::ptrdiff_t;
  using propagate_on_container_move_assignment = ::std::true_type;
  using is_always_equal = ::std::true_type;

  template <class U> struct rebind {
    using other = AlignedAllocator<U, Alignment>;
  };

  constexpr AlignedAllocator() noexcept = default;

  template <class U>
  constexpr AlignedAllocator(const AlignedAllocator<U, Alignment> &) noexcept {}

  [[nodiscard]] T *allocate(size_type count) {
    if (count > max_size())
      throw ::std::bad_array_new_length();
    return static_cast<T *>(
        ::operator new(count * sizeof(T), ::std::align_val_t{Alignment}));
  }

  void deallocate(T *ptr, size_type) noexcept {
    ::operator delete(ptr, ::std::align_val_t{Alignment});
  }

  constexpr size_type max_size() const noexcept {
    return static_cast<size_type>(-1) / sizeof(T);
  }
};

template <class T, class U, ::std::size_t Alignment>
constexpr bool operator==(const AlignedAllocator<T, Alignment> &,
                          const AlignedAllocator<U, Alignment> &) noexcept {
  return true;
}

} // namespace pluss::coden::detail
