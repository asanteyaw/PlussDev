#pragma once

#include <array>
#include <cstdint>
#include <initializer_list>
#include <span>
#include <vector>

#include "CodeH/core/Errors.h"
#include "MSharp/RangeView.h"
#include "pluss/codeh/internal/timeline_factory_detail.h"
#include "pluss/detail/nested_timeline_literal.h"

namespace pluss::codeh::detail {

// Shared exact "flat int64 + shape" ingestion for Duration's unit factories
// and Timeline::from_epoch -- deliberately NOT built on
// pluss::detail::DataReader (Series' engine-level reader). That type's
// scalar/init-list/nested-literal paths all round-trip every value through
// double (NestedSeriesLiteral::scalar is a double), which silently corrupts
// int64 magnitudes beyond 2^53 -- verified directly: pluss::make_series with
// a nested-braced or scalar int64 literal at 2^62+1 comes back off by one.
// Nanosecond counts routinely exceed 2^53, so this reader stays int64
// end-to-end at every step: NestedInt64Literal already does (its `scalar`
// member is std::int64_t), and the span/RangeView paths are plain copies,
// so nothing here ever touches a floating type.
class Int64FlatReader {
 public:
  // Scalar (0D).
  Int64FlatReader(std::int64_t v) : flat_{v} {}

  // Nested braces -- shape inferred from nesting depth (0D..3D). A flat
  // brace list like {1,2,3} matches this too (NestedInt64Literal converts
  // implicitly from any integral scalar), giving rank 1.
  Int64FlatReader(std::initializer_list<pluss::detail::NestedInt64Literal> nested) {
    auto lit = pluss::detail::flatten_int64_literal(nested, 3, "pluss int64 factory");
    flat_ = std::move(lit.flat);
    sizes_ = std::move(lit.sizes);
  }

  // Bare 1D data (an existing span/vector/RangeView, not a brace literal) --
  // shape is implicitly [n]. Exact: these are plain copies of existing
  // int64_t storage, no reinterpretation. std::span and std::vector each get
  // their own direct constructor rather than relying on the RangeView one
  // below: going through RangeView as an intermediate would chain two
  // user-defined conversions (span/vector -> RangeView -> Int64FlatReader),
  // which C++ won't do implicitly -- same reason TimelineTextReader needs a
  // direct std::string constructor instead of relying on its string_view one.
  Int64FlatReader(std::span<const std::int64_t> flat)
      : flat_(flat.begin(), flat.end()),
        sizes_{static_cast<std::int64_t>(flat.size())} {}
  Int64FlatReader(const std::vector<std::int64_t>& flat)
      : Int64FlatReader(std::span<const std::int64_t>(flat.data(), flat.size())) {}
  Int64FlatReader(pluss::msharp::RangeView::Of<std::int64_t> flat)
      : flat_(flat.begin(), flat.end()),
        sizes_{static_cast<std::int64_t>(flat.size())} {}

  // Explicit flat + shape, for cases the nested-literal syntax can't express
  // conveniently (already-flat runtime data). Rank-checked here (matching
  // what the nested-literal path already self-enforces via flatten_int64_literal's
  // own max_rank), since this is the one path where a caller could otherwise
  // pass a shape with rank > 3 undetected until some later Duration/Timeline op.
  Int64FlatReader(pluss::msharp::RangeView::Of<std::int64_t> flat,
                  pluss::msharp::RangeView::Of<std::int64_t> sizes)
      : flat_(flat.begin(), flat.end()), sizes_(sizes.begin(), sizes.end()) {
    pluss::codeh::factory_detail::check_rank(
        pluss::detail::ShapeSpec(std::span<const std::int64_t>(sizes_.data(), sizes_.size())),
        "pluss int64 factory (flat, sizes)");
  }

  std::span<const std::int64_t> flat() const noexcept { return flat_; }
  std::span<const std::int64_t> sizes() const noexcept { return sizes_; }

 private:
  std::vector<std::int64_t> flat_;
  std::vector<std::int64_t> sizes_;  // empty = scalar (0D)
};

} // namespace pluss::codeh::detail
