#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "MSharp/RangeView.h"
#include "pluss/codeh/codeh.h"
#include "pluss/codeh/duration_factory.h"
#include "pluss/codeh/internal/timeline_factory_detail.h"
#include "pluss/detail/factory_containers.h"
#include "pluss/detail/nested_timeline_literal.h"

#include "CodeH/compat/Invoke.h"

namespace pluss {

// ---- Public factories (preferred construction path) ----
//
// make_timeline("2026-02-15")          -> scalar Timeline (0D)
// make_timeline({"2026-02-15","2026-02-16"}) -> 1D Timeline
// make_timeline({{{...}}})             -> up to 3D Timeline via nested literal
// make_timeline(flat, sizes)           -> N-D Timeline (0D..3D)
// from_epoch_nanos(...)               -> from int64 epoch nanos
//
// Duration has its own dedicated factory namespace, `pluss::duration::`
// (`pluss::duration::seconds(5)`, `pluss::duration::range(...)`, etc.) --
// see duration_factory.h, included above for transitive availability.

inline pluss::codeh::Timeline make_timeline(std::string_view s) {
  const std::array<std::string_view, 1> flat{s};
  return pluss::codeh::ops_internal::timeline_from_utf8(flat, {});
}
inline pluss::codeh::Timeline make_timeline(const char* s) {
  return make_timeline(std::string_view{s ? s : ""});
}
inline pluss::codeh::Timeline make_timeline(std::initializer_list<const char*> s1d) {
  std::vector<std::string_view> views;
  views.reserve(s1d.size());
  for (const char* s : s1d) {
    views.emplace_back(s ? s : "");
  }
  return codeh::factory_detail::from_textspan_1d(detail::TextSpan(views));
}
inline pluss::codeh::Timeline make_timeline(pluss::msharp::RangeView::Of<std::string_view> s1d) {
  return codeh::factory_detail::from_textspan_1d(
      detail::TextSpan(std::span<const std::string_view>(s1d.data(), s1d.size())));
}
inline pluss::codeh::Timeline make_timeline(pluss::msharp::RangeView::Of<std::string> s1d) {
  return codeh::factory_detail::from_textspan_1d(
      detail::TextSpan::from_strings(std::span<const std::string>(s1d.data(), s1d.size())));
}
inline pluss::codeh::Timeline make_timeline(std::initializer_list<detail::NestedTimelineLiteral> nested) {
  auto lit = detail::flatten_timeline_literal(nested, 3, "make_timeline");
  return codeh::factory_detail::from_textspan(
      detail::TextSpan::from_strings(lit.flat),
      detail::ShapeSpec(std::span<const std::int64_t>(lit.sizes.data(), lit.sizes.size())));
}

inline pluss::codeh::Timeline make_timeline(pluss::msharp::RangeView::Of<std::string_view> flat,
                                            pluss::msharp::RangeView::Of<std::int64_t> sizes) {
  return codeh::factory_detail::from_textspan(
      detail::TextSpan(std::span<const std::string_view>(flat.data(), flat.size())),
      detail::ShapeSpec(std::span<const std::int64_t>(sizes.data(), sizes.size())));
}

inline pluss::codeh::Timeline make_timeline(pluss::msharp::RangeView::Of<std::string> flat,
                                            pluss::msharp::RangeView::Of<std::int64_t> sizes) {
  return codeh::factory_detail::from_textspan(
      detail::TextSpan::from_strings(std::span<const std::string>(flat.data(), flat.size())),
      detail::ShapeSpec(std::span<const std::int64_t>(sizes.data(), sizes.size())));
}

inline pluss::codeh::Timeline from_epoch_nanos(std::int64_t scalar) {
  const std::array<std::int64_t, 1> flat{scalar};
  return pluss::codeh::ops_internal::timeline_from_epoch_nanos(flat, {});
}

inline pluss::codeh::Timeline from_epoch_nanos(
    std::initializer_list<detail::NestedInt64Literal> nested) {
  auto lit = detail::flatten_int64_literal(nested, 3, "from_epoch_nanos");
  return pluss::codeh::ops_internal::timeline_from_epoch_nanos(
      std::span<const std::int64_t>(lit.flat.data(), lit.flat.size()),
      std::span<const std::int64_t>(lit.sizes.data(), lit.sizes.size()));
}

inline pluss::codeh::Timeline from_epoch_nanos(pluss::msharp::RangeView::Of<std::int64_t> flat,
                                               pluss::msharp::RangeView::Of<std::int64_t> sizes) {
  const detail::ShapeSpec shape(std::span<const std::int64_t>(sizes.data(), sizes.size()));
  codeh::factory_detail::check_rank(shape, "from_epoch_nanos");
  return pluss::codeh::ops_internal::timeline_from_epoch_nanos(
      std::span<const std::int64_t>(flat.data(), flat.size()),
      shape.span());
}

inline pluss::codeh::Timeline date_range(const pluss::codeh::Timeline& start,
                                         const pluss::codeh::Timeline& end,
                                         std::string_view freq,
                                         std::int64_t step = 1) {
  return ::pluss::codeh::_compat::invoke<pluss::codeh::Timeline>("date_range", start, {end, std::string(freq), step});
}

} // namespace pluss

// Keep moment helpers transitively available from timeline_factory.
#include "pluss/codeh/timeline_moment.h"
