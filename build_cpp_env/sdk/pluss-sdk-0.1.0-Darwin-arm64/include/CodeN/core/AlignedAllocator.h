#pragma once

#include <cstddef>
#include <cstdint>
#include <new>
#include <type_traits>

namespace pluss::coden::detail {

struct LaneAllocationSnapshot {
  ::std::uint64_t allocations{0};
  ::std::uint64_t bytes{0};
};

inline thread_local LaneAllocationSnapshot* active_lane_allocation_stats = nullptr;

class LaneAllocationScope {
public:
  LaneAllocationScope() noexcept
      : previous_(active_lane_allocation_stats) {
    active_lane_allocation_stats = &stats_;
  }
  ~LaneAllocationScope() { active_lane_allocation_stats = previous_; }

  LaneAllocationScope(const LaneAllocationScope&) = delete;
  LaneAllocationScope& operator=(const LaneAllocationScope&) = delete;

  LaneAllocationSnapshot snapshot() const noexcept { return stats_; }

private:
  LaneAllocationSnapshot stats_{};
  LaneAllocationSnapshot* previous_{nullptr};
};

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
    if (active_lane_allocation_stats != nullptr) {
      ++active_lane_allocation_stats->allocations;
      active_lane_allocation_stats->bytes += count * sizeof(T);
    }
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
