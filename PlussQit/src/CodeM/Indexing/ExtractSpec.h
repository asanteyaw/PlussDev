#pragma once

#include <cstdint>
#include <initializer_list>
#include <type_traits>

#include "CodeM/core/Errors.h"
#include "MSharp/IntrusivePtr.h"
#include "pluss/slicing.h"

namespace pluss::codem {

class Series;
class SeriesImpl;

struct ExtractSpec final {
  enum class Kind : std::uint8_t { Slice, ScalarIndex, AdvancedIndex, NewAxis, Ellipsis };

  Kind kind{Kind::Slice};
  pluss::Slicing::Slice slice{};
  std::int64_t scalar_index{0};
  pluss::msharp::intrusive_ptr<SeriesImpl> advanced_impl{};

  ExtractSpec() = default;
  ExtractSpec(pluss::Slicing::Slice s) : kind(Kind::Slice), slice(s) {}
  ExtractSpec(pluss::Slicing::EllipsisTag) : kind(Kind::Ellipsis) {}
  ExtractSpec(pluss::Slicing::NewAxisTag) : kind(Kind::NewAxis) {}

  ExtractSpec(std::initializer_list<std::int64_t> shorthand)
      : kind(Kind::Slice), slice(slice_from_shorthand_(shorthand)) {}

  template <class I>
    requires(std::is_integral_v<std::remove_cv_t<I>> && !std::is_same_v<std::remove_cv_t<I>, bool>)
  ExtractSpec(I i) : kind(Kind::ScalarIndex), scalar_index(static_cast<std::int64_t>(i)) {}

  ExtractSpec(const Series& idx);

private:
  static pluss::Slicing::Slice slice_from_shorthand_(std::initializer_list<std::int64_t> shorthand) {
    PLUSS_CHECK(shorthand.size() >= 1 && shorthand.size() <= 3,
                "Series::extract: slice shorthand must have 1, 2, or 3 elements");
    const auto* p = shorthand.begin();
    if (shorthand.size() == 1) return pluss::Slicing::Slice{p[0]};
    if (shorthand.size() == 2) return pluss::Slicing::SliceRange(p[0], p[1]);
    return pluss::Slicing::SliceRange(p[0], p[1], p[2]);
  }
};

} // namespace pluss::codem