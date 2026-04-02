#pragma once

#include <cstdint>
#include <limits>

namespace pluss::Slicing {

struct Slice final {
  static constexpr std::int64_t kUnbounded = std::numeric_limits<std::int64_t>::min();

  std::int64_t start{kUnbounded};
  std::int64_t stop{kUnbounded};
  std::int64_t step{1};

  constexpr Slice() = default;
  constexpr Slice(std::int64_t stop_) : start(kUnbounded), stop(stop_), step(1) {}
  constexpr Slice(std::int64_t start_, std::int64_t stop_, std::int64_t step_ = 1)
      : start(start_), stop(stop_), step(step_) {}

  constexpr bool is_all() const { return start == kUnbounded && stop == kUnbounded && step == 1; }
};

struct EllipsisTag final {};
struct NewAxisTag final {};

inline constexpr Slice SliceAll() {
  return Slice{};
}

inline constexpr Slice SliceFrom(std::int64_t start, std::int64_t step = 1) {
  Slice s;
  s.start = start;
  s.step = step;
  return s;
}

inline constexpr Slice SliceTo(std::int64_t stop, std::int64_t step = 1) {
  Slice s;
  s.stop = stop;
  s.step = step;
  return s;
}

inline constexpr Slice SliceRange(std::int64_t start, std::int64_t stop, std::int64_t step = 1) {
  return Slice{start, stop, step};
}

inline constexpr EllipsisTag Ellipsis() {
  return EllipsisTag{};
}

inline constexpr NewAxisTag NewAxis() {
  return NewAxisTag{};
}

} // namespace pluss::Slicing
