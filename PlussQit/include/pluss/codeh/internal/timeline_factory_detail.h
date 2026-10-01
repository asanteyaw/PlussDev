#pragma once

#include <array>
#include <cctype>
#include <cstdint>
#include <random>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "pluss/codeh/codeh.h"
#include "pluss/detail/factory_containers.h"
#include "pluss/detail/factory_core.h"

namespace pluss::codeh::factory_detail {

inline void check_rank(pluss::detail::ShapeSpec sizes, std::string_view where) {
  CODEM_CHECK(sizes.rank() <= 3, std::string(where) + ": supports ranks up to 3");
}

inline std::int64_t checked_numel(pluss::msharp::RangeView::Int sizes,
                                  std::string_view where) {
  CODEM_CHECK(sizes.size() <= 3, std::string(where) + ": supports ranks up to 3");
  std::int64_t result = 1;
  for (const auto extent : sizes) {
    CODEM_CHECK(extent >= 0, std::string(where) + ": shape extents must be non-negative");
    CODEM_CHECK(extent == 0 || result <= std::numeric_limits<std::int64_t>::max() / extent,
                std::string(where) + ": shape element count overflows int64");
    result *= extent;
  }
  return result;
}

inline pluss::codeh::Timeline from_textspan(pluss::detail::TextSpan flat,
                                            pluss::detail::ShapeSpec sizes) {
  check_rank(sizes, "make_timeline");
  return pluss::codeh::ops_internal::timeline_from_utf8(flat.span(), sizes.span());
}

inline pluss::codeh::Timeline from_textspan_1d(pluss::detail::TextSpan s1d) {
  const std::array<std::int64_t, 1> sizes{static_cast<std::int64_t>(s1d.size())};
  return from_textspan(
      std::move(s1d),
      pluss::detail::ShapeSpec(std::span<const std::int64_t>(sizes.data(), sizes.size())));
}

inline std::string trim_ascii(std::string_view s) {
  std::size_t b = 0;
  std::size_t e = s.size();
  while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
  return std::string(s.substr(b, e - b));
}

enum class TimelineTextKind : std::uint8_t {
  Unknown = 0,
  DateOnly = 1,
  TimeOnly = 2,
  DateTime = 3,
};

inline bool looks_like_date_only(std::string_view s) {
  return s.size() == 10 &&
      std::isdigit(static_cast<unsigned char>(s[0])) &&
      std::isdigit(static_cast<unsigned char>(s[1])) &&
      std::isdigit(static_cast<unsigned char>(s[2])) &&
      std::isdigit(static_cast<unsigned char>(s[3])) &&
      (s[4] == '-' || s[4] == '/') &&
      std::isdigit(static_cast<unsigned char>(s[5])) &&
      std::isdigit(static_cast<unsigned char>(s[6])) &&
      (s[7] == '-' || s[7] == '/') &&
      std::isdigit(static_cast<unsigned char>(s[8])) &&
      std::isdigit(static_cast<unsigned char>(s[9]));
}

inline TimelineTextKind classify_text(std::string_view token) {
  const auto s = trim_ascii(token);
  if (s.empty()) return TimelineTextKind::Unknown;
  if (s.find('T') != std::string::npos) return TimelineTextKind::DateTime;
  if (looks_like_date_only(s)) return TimelineTextKind::DateOnly;
  if (s.find(':') != std::string::npos) return TimelineTextKind::TimeOnly;
  return TimelineTextKind::Unknown;
}

inline std::string require_date_token(std::string_view token, std::string_view where) {
  const auto s = trim_ascii(token);
  CODEM_CHECK(classify_text(s) == TimelineTextKind::DateOnly,
              std::string(where) + ": expected a date-only token");
  return s;
}

inline std::string require_time_token(std::string_view token, std::string_view where) {
  const auto s = trim_ascii(token);
  CODEM_CHECK(classify_text(s) == TimelineTextKind::TimeOnly,
              std::string(where) + ": expected a time-only token");
  return "1970-01-01T" + s;
}

inline pluss::codeh::Timeline from_date_textspan(pluss::detail::TextSpan flat,
                                                 pluss::detail::ShapeSpec sizes) {
  check_rank(sizes, "pluss::timeline::date");
  std::vector<std::string> owned;
  owned.reserve(flat.size());
  for (const auto item : flat.span()) {
    owned.push_back(require_date_token(item, "pluss::timeline::date"));
  }
  return from_textspan(pluss::detail::TextSpan::from_strings(owned), sizes);
}

inline pluss::codeh::Timeline from_time_textspan(pluss::detail::TextSpan flat,
                                                 pluss::detail::ShapeSpec sizes) {
  check_rank(sizes, "pluss::timeline::time");
  std::vector<std::string> owned;
  owned.reserve(flat.size());
  for (const auto item : flat.span()) {
    owned.push_back(require_time_token(item, "pluss::timeline::time"));
  }
  return from_textspan(pluss::detail::TextSpan::from_strings(owned), sizes);
}

inline pluss::codeh::Timeline random_between_inclusive(pluss::msharp::RangeView::Int sizes,
                                                       const pluss::codeh::Timeline& lo,
                                                       const pluss::codeh::Timeline& hi,
                                                       std::uint64_t seed = 0) {
  CODEM_CHECK(lo.dim() == 0 && hi.dim() == 0,
              "pluss::timeline::rand_between: lo/hi must be scalar Timelines");
  const std::int64_t lo_ns = lo.ticks()[0];
  const std::int64_t hi_ns = hi.ticks()[0];
  CODEM_CHECK(lo_ns != pluss::codeh::Timeline::kNaT && hi_ns != pluss::codeh::Timeline::kNaT,
              "pluss::timeline::rand_between: NaT bounds are unsupported");
  CODEM_CHECK(hi_ns >= lo_ns, "pluss::timeline::rand_between: expected hi >= lo");
  const auto n = checked_numel(sizes, "pluss::timeline::rand_between");
  auto rng = pluss::detail::FactoryCore::make_rng(seed);
  std::uniform_int_distribution<std::int64_t> dist(lo_ns, hi_ns);
  std::vector<std::int64_t> flat(static_cast<std::size_t>(n));
  for (auto& v : flat) v = dist(rng);
  auto out = pluss::codeh::ops_internal::timeline_from_epoch_nanos(flat, sizes);
  return pluss::codeh::ops_internal::timeline_from_series(out.unsafe_ticks().clone(), std::string(lo.tz()));
}

} // namespace pluss::codeh::factory_detail
