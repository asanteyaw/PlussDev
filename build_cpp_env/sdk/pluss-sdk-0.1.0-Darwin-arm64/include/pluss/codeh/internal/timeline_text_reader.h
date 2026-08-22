#pragma once

#include <array>
#include <cstdint>
#include <initializer_list>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "MSharp/RangeView.h"
#include "pluss/codeh/internal/timeline_factory_detail.h"
#include "pluss/detail/factory_containers.h"
#include "pluss/detail/nested_timeline_literal.h"

namespace pluss::codeh::detail {

// Normalizes every Timeline::make/TimeIndex::make input shape (scalar, 1D
// list, nested-braced N-D, or explicit flat+shape) into one flat
// pluss::detail::TextSpan + pluss::detail::ShapeSpec pair, reusing the
// existing, already-tested TextSpan/ShapeSpec/flatten_timeline_literal/
// check_rank machinery rather than re-deriving it -- this is a text-domain
// counterpart to Int64FlatReader, not built on it, since strings and int64
// counts are different domains (see int64_flat_reader.h for why the numeric
// side isn't just Series' DataReader either).
class TimelineTextReader {
 public:
  // Scalar (0D). string_view is a non-owning view; safe here because the
  // reader is always consumed synchronously within the same full expression
  // as the pluss::timeline::make(...)/TimeIndex::make(...) call that builds
  // it (same lifetime envelope the original scalar overload relied on).
  TimelineTextReader(std::string_view s) : flat_(std::vector<std::string_view>{s}) {}
  TimelineTextReader(const char* s) : TimelineTextReader(std::string_view{s ? s : ""}) {}
  // std::string -> std::string_view -> TimelineTextReader would chain two
  // user-defined conversions, which C++ won't do implicitly -- needs its
  // own direct constructor rather than relying on the string_view one above.
  TimelineTextReader(const std::string& s) : TimelineTextReader(std::string_view{s}) {}

  // 1D. std::span/std::vector each get direct constructors rather than
  // relying on the RangeView ones below -- routing a caller's raw
  // span/vector through RangeView first would chain two user-defined
  // conversions (span/vector -> RangeView -> reader), which C++ won't do
  // implicitly (same reason Int64FlatReader needs direct span/vector
  // constructors too).
  TimelineTextReader(std::initializer_list<const char*> s1d) {
    assign_1d_(pluss::detail::TextSpan(s1d));
  }
  TimelineTextReader(std::span<const std::string_view> s1d) {
    assign_1d_(pluss::detail::TextSpan(s1d));
  }
  TimelineTextReader(const std::vector<std::string_view>& s1d) {
    assign_1d_(pluss::detail::TextSpan(s1d));
  }
  TimelineTextReader(const std::vector<std::string>& s1d) {
    assign_1d_(pluss::detail::TextSpan::from_strings(s1d));
  }
  TimelineTextReader(pluss::msharp::RangeView::Of<std::string_view> s1d) {
    assign_1d_(pluss::detail::TextSpan(std::span<const std::string_view>(s1d.data(), s1d.size())));
  }
  TimelineTextReader(pluss::msharp::RangeView::Of<std::string> s1d) {
    assign_1d_(pluss::detail::TextSpan::from_strings(std::span<const std::string>(s1d.data(), s1d.size())));
  }

  // Nested braces -- shape inferred from nesting depth (0D..3D). Builds
  // *owned* string storage (flatten_timeline_literal materializes fresh
  // std::strings), which must outlive flat_'s string_views into it --
  // owned_strings_ is populated first, then flat_ is built from it, in
  // that order, in this constructor body.
  TimelineTextReader(std::initializer_list<pluss::detail::NestedTimelineLiteral> nested) {
    auto lit = pluss::detail::flatten_timeline_literal(nested, 3, "pluss::timeline::make");
    owned_strings_ = std::move(lit.flat);
    flat_ = pluss::detail::TextSpan::from_strings(owned_strings_);
    shape_ = pluss::detail::ShapeSpec(std::move(lit.sizes));
  }

  // Explicit flat + shape, two source shapes. Rank-checked, matching what
  // the nested-literal path already self-enforces via flatten_timeline_literal's
  // own max_rank.
  TimelineTextReader(pluss::msharp::RangeView::Of<std::string_view> flat,
                     pluss::msharp::RangeView::Of<std::int64_t> sizes) {
    flat_ = pluss::detail::TextSpan(std::span<const std::string_view>(flat.data(), flat.size()));
    assign_shape_(sizes);
  }
  TimelineTextReader(pluss::msharp::RangeView::Of<std::string> flat,
                     pluss::msharp::RangeView::Of<std::int64_t> sizes) {
    flat_ = pluss::detail::TextSpan::from_strings(std::span<const std::string>(flat.data(), flat.size()));
    assign_shape_(sizes);
  }

  const pluss::detail::TextSpan& flat() const noexcept { return flat_; }
  const pluss::detail::ShapeSpec& shape() const noexcept { return shape_; }

 private:
  void assign_1d_(pluss::detail::TextSpan flat) {
    const std::array<std::int64_t, 1> sz{static_cast<std::int64_t>(flat.size())};
    flat_ = std::move(flat);
    shape_ = pluss::detail::ShapeSpec(std::span<const std::int64_t>(sz.data(), sz.size()));
  }
  void assign_shape_(pluss::msharp::RangeView::Of<std::int64_t> sizes) {
    shape_ = pluss::detail::ShapeSpec(std::span<const std::int64_t>(sizes.data(), sizes.size()));
    pluss::codeh::factory_detail::check_rank(shape_, "pluss::timeline::make (flat, sizes)");
  }

  std::vector<std::string> owned_strings_;  // backing storage for the nested-literal path only
  pluss::detail::TextSpan flat_;
  pluss::detail::ShapeSpec shape_;  // empty = scalar (0D)
};

} // namespace pluss::codeh::detail
