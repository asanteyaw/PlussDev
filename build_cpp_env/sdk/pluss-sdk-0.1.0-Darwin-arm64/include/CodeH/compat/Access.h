#pragma once

#include <algorithm>
#include <chrono>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

#include "CodeH/core/TimeIndex.h"
#include "CodeH/core/TimeZone.h"
#include "CodeH/compat/Invoke.h"

namespace pluss::codeh::_compat::detail {

struct ParsedResampleFreq {
  enum class Kind : std::uint8_t {
    Fixed = 0,
    Week = 1,
    Month = 2,
    Year = 3,
  };

  Kind kind{Kind::Fixed};
  std::int64_t multiplier{1};
  std::int64_t unit_nanos{0};
};

static std::string lower_ascii_copy_(std::string_view s) {
  std::string out(s);
  for (auto& ch : out) {
    if (ch >= 'A' && ch <= 'Z') ch = static_cast<char>(ch - 'A' + 'a');
  }
  return out;
}

static ParsedResampleFreq parse_resample_freq_(std::string_view raw_freq, std::int64_t step) {
  CODEM_CHECK(step > 0, "resample_bins: step must be positive");
  CODEM_CHECK(!raw_freq.empty(), "resample_bins: freq cannot be empty");

  std::size_t pos = 0;
  std::int64_t prefix = 0;
  while (pos < raw_freq.size() && raw_freq[pos] >= '0' && raw_freq[pos] <= '9') {
    prefix = prefix * 10 + static_cast<std::int64_t>(raw_freq[pos] - '0');
    ++pos;
  }
  if (prefix == 0) prefix = 1;
  const auto mult = prefix * step;
  const auto unit = raw_freq.substr(pos);
  CODEM_CHECK(!unit.empty(), "resample_bins: freq unit cannot be empty");
  const auto unit_lower = lower_ascii_copy_(unit);

  if (unit == "S" || unit_lower == "s" || unit_lower == "sec" || unit_lower == "second" || unit_lower == "seconds") {
    return {ParsedResampleFreq::Kind::Fixed, mult, 1'000'000'000LL};
  }
  if (unit_lower == "min" || unit_lower == "minute" || unit_lower == "minutes" || unit_lower == "t" || unit == "m") {
    return {ParsedResampleFreq::Kind::Fixed, mult, 60LL * 1'000'000'000LL};
  }
  if (unit == "H" || unit_lower == "h" || unit_lower == "hr" || unit_lower == "hour" || unit_lower == "hours") {
    return {ParsedResampleFreq::Kind::Fixed, mult, 3600LL * 1'000'000'000LL};
  }
  if (unit == "D" || unit_lower == "d" || unit_lower == "day" || unit_lower == "days") {
    return {ParsedResampleFreq::Kind::Fixed, mult, 86400LL * 1'000'000'000LL};
  }
  if (unit == "W" || unit_lower == "w" || unit_lower == "week" || unit_lower == "weeks") {
    return {ParsedResampleFreq::Kind::Week, mult, 7LL * 86400LL * 1'000'000'000LL};
  }
  if (unit == "M" || unit == "MS" || unit_lower == "mo" || unit_lower == "mth"
      || unit_lower == "month" || unit_lower == "months") {
    return {ParsedResampleFreq::Kind::Month, mult, 0};
  }
  if (unit == "Y" || unit == "YS" || unit_lower == "y" || unit_lower == "yr"
      || unit_lower == "year" || unit_lower == "years" || unit_lower == "annual") {
    return {ParsedResampleFreq::Kind::Year, mult, 0};
  }
  CODEM_FAIL("TimeIndex: unsupported freq (expected fixed units like S/min/H/D, weekly W, monthly M, yearly Y)");
}

static Timeline parse_scalar_iso_(std::string_view s) {
  const std::string_view flat[1] = {s};
  return ::pluss::codeh::ops_internal::timeline_from_utf8(flat, {});
}

static std::int64_t floor_div_i64_(std::int64_t a, std::int64_t b) {
  CODEM_CHECK(b > 0, "floor_div_i64_: divisor must be positive");
  std::int64_t q = a / b;
  const std::int64_t r = a % b;
  if (r != 0 && r < 0) --q;
  return q;
}

static ::pluss::codeh::TimeIndex::ResampleOptions resample_options_(
    std::string_view freq,
    std::int64_t step,
    ::pluss::codeh::TimeIndex::ResampleClosed closed =
        ::pluss::codeh::TimeIndex::ResampleClosed::Left,
    ::pluss::codeh::TimeIndex::ResampleLabel label =
        ::pluss::codeh::TimeIndex::ResampleLabel::Left,
    ::pluss::codeh::TimeIndex::ResampleOrigin origin =
        ::pluss::codeh::TimeIndex::ResampleOrigin::StartDay,
    std::int64_t offset_nanos = 0) {
  return ::pluss::codeh::TimeIndex::ResampleOptions{
      .freq = std::string(freq),
      .step = step,
      .closed = closed,
      .label = label,
      .origin = origin,
      .offset_nanos = offset_nanos,
  };
}

static void check_values_1d_match_(const pluss::codem::Series& v, std::int64_t n) {
  CODEM_CHECK(v.defined(), "values undefined");
  CODEM_CHECK(v.dim() == 1, "values must be 1D");
  CODEM_CHECK(v.numel() == n, "values length must equal index length");
  CODEM_CHECK(v.scalar_type() != pluss::codem::ScalarType::Bool, "values must be numeric (not bool)");
}

struct Access {
  // Create (private) TimeIndex
  static ::pluss::codeh::TimeIndex make(::pluss::codeh::Timeline t) { return ::pluss::codeh::TimeIndex(std::move(t)); }

  static const ::pluss::codeh::Timeline& timeline(const ::pluss::codeh::TimeIndex& idx) { return idx.t_; }

  static bool is_monotonic_increasing(const ::pluss::codeh::TimeIndex& self) {
    const auto* x = reinterpret_cast<const std::int64_t*>(self.t_.ticks_.raw_data());
    for (std::int64_t i = 1; i < self.size(); ++i) if (x[i] < x[i-1]) return false;
    return true;
  }

  static bool is_monotonic_decreasing(const ::pluss::codeh::TimeIndex& self) {
    const auto* x = reinterpret_cast<const std::int64_t*>(self.t_.ticks_.raw_data());
    for (std::int64_t i = 1; i < self.size(); ++i) if (x[i] > x[i-1]) return false;
    return true;
  }

  static bool is_unique(const ::pluss::codeh::TimeIndex& self) {
    const auto* x = reinterpret_cast<const std::int64_t*>(self.t_.ticks_.raw_data());
    for (std::int64_t i = 1; i < self.size(); ++i) if (x[i] == x[i-1]) return false;
    return true;
  }

  static ::pluss::codeh::Timeline min(const ::pluss::codeh::TimeIndex& self) {
    return ::pluss::codeh::detail::impl_min(self.t_, true);
  }
  static ::pluss::codeh::Timeline max(const ::pluss::codeh::TimeIndex& self) {
    return ::pluss::codeh::detail::impl_max(self.t_, true);
  }

  static ::pluss::codeh::TimeIndex sort(const ::pluss::codeh::TimeIndex& self) {
    return make(::pluss::codeh::detail::impl_sort(self.t_));
  }

  static ::pluss::codeh::TimeIndex unique(const ::pluss::codeh::TimeIndex& self) {
    return make(::pluss::codeh::detail::impl_unique(self.t_));
  }

  static pluss::codem::Series argsort(const ::pluss::codeh::TimeIndex& self) {
    return ::pluss::codeh::detail::impl_argsort(self.t_);
  }

  static ::pluss::codeh::TimeIndex take(const ::pluss::codeh::TimeIndex& self, const pluss::codem::Series& indices) {
    CODEM_CHECK(indices.defined(), "TimeIndex::take: indices undefined");
    CODEM_CHECK(indices.scalar_type() == pluss::codem::ScalarType::Int64, "TimeIndex::take: indices must be int64");
    CODEM_CHECK(indices.dim() == 1, "TimeIndex::take: indices must be 1D");
    const auto* ix = reinterpret_cast<const std::int64_t*>(indices.raw_data());
    const auto n = indices.numel();
    auto out = pluss::codem::Series::empty({n}, pluss::codem::ScalarType::Int64);
    auto* o = reinterpret_cast<std::int64_t*>(out.raw_data());
    const auto* x = reinterpret_cast<const std::int64_t*>(self.t_.ticks_.raw_data());
    for (std::int64_t i = 0; i < n; ++i) {
      const auto j = ix[i];
      CODEM_CHECK(j >= 0 && j < self.size(), "TimeIndex::take: index out of range");
      o[i] = x[j];
    }
    return make(::pluss::codeh::Timeline(std::move(out), std::string(self.t_.tz())));
  }

  static ::pluss::codeh::TimeIndex slice_index(const ::pluss::codeh::TimeIndex& self, std::int64_t start, std::int64_t stop) {
    const auto n = self.size();
    if (start < 0) start = 0;
    if (stop > n) stop = n;
    if (stop < start) stop = start;
    const auto len = stop - start;
    auto out = pluss::codem::Series::empty({len}, pluss::codem::ScalarType::Int64);
    auto* o = reinterpret_cast<std::int64_t*>(out.raw_data());
    const auto* x = reinterpret_cast<const std::int64_t*>(self.t_.ticks_.raw_data());
    for (std::int64_t i = 0; i < len; ++i) o[i] = x[start + i];
    return make(::pluss::codeh::Timeline(std::move(out), std::string(self.t_.tz())));
  }

  static ::pluss::codeh::TimeIndex tail(const ::pluss::codeh::TimeIndex& self, std::int64_t n) {
    const auto N = self.size();
    if (n < 0) n = 0;
    if (n > N) n = N;
    return slice_index(self, N - n, N);
  }

  static ::pluss::codeh::TimeIndex union_(const ::pluss::codeh::TimeIndex& a, const ::pluss::codeh::TimeIndex& b) {
    // Both must be sorted and same tz.
    CODEM_CHECK(std::string_view(a.t_.tz()) == std::string_view(b.t_.tz()), "TimeIndex::union: tz mismatch");
    const auto* x = reinterpret_cast<const std::int64_t*>(a.t_.ticks_.raw_data());
    const auto* y = reinterpret_cast<const std::int64_t*>(b.t_.ticks_.raw_data());
    std::vector<std::int64_t> outv;
    outv.reserve((size_t)(a.size() + b.size()));
    std::int64_t i = 0, j = 0;
    while (i < a.size() || j < b.size()) {
      std::int64_t v;
      if (j >= b.size() || (i < a.size() && x[i] < y[j])) v = x[i++];
      else if (i >= a.size() || (j < b.size() && y[j] < x[i])) v = y[j++];
      else { v = x[i]; ++i; ++j; }
      if (outv.empty() || outv.back() != v) outv.push_back(v);
    }
    auto out = pluss::codem::Series::empty({(std::int64_t)outv.size()}, pluss::codem::ScalarType::Int64);
    std::memcpy(out.raw_data(), outv.data(), outv.size() * sizeof(std::int64_t));
    return make(::pluss::codeh::Timeline(std::move(out), std::string(a.t_.tz())));
  }

  static ::pluss::codeh::TimeIndex intersection(const ::pluss::codeh::TimeIndex& a, const ::pluss::codeh::TimeIndex& b) {
    CODEM_CHECK(std::string_view(a.t_.tz()) == std::string_view(b.t_.tz()), "TimeIndex::intersection: tz mismatch");
    const auto* x = reinterpret_cast<const std::int64_t*>(a.t_.ticks_.raw_data());
    const auto* y = reinterpret_cast<const std::int64_t*>(b.t_.ticks_.raw_data());
    std::vector<std::int64_t> outv;
    std::int64_t i = 0, j = 0;
    while (i < a.size() && j < b.size()) {
      if (x[i] < y[j]) ++i;
      else if (y[j] < x[i]) ++j;
      else { outv.push_back(x[i]); ++i; ++j; }
    }
    auto out = pluss::codem::Series::empty({(std::int64_t)outv.size()}, pluss::codem::ScalarType::Int64);
    if (!outv.empty()) std::memcpy(out.raw_data(), outv.data(), outv.size() * sizeof(std::int64_t));
    return make(::pluss::codeh::Timeline(std::move(out), std::string(a.t_.tz())));
  }

  static ::pluss::codeh::TimeIndex difference(const ::pluss::codeh::TimeIndex& a, const ::pluss::codeh::TimeIndex& b) {
    CODEM_CHECK(std::string_view(a.t_.tz()) == std::string_view(b.t_.tz()), "TimeIndex::difference: tz mismatch");
    const auto* x = reinterpret_cast<const std::int64_t*>(a.t_.ticks_.raw_data());
    const auto* y = reinterpret_cast<const std::int64_t*>(b.t_.ticks_.raw_data());
    std::vector<std::int64_t> outv;
    std::int64_t i = 0, j = 0;
    while (i < a.size()) {
      while (j < b.size() && y[j] < x[i]) ++j;
      if (j < b.size() && y[j] == x[i]) { ++i; continue; }
      outv.push_back(x[i]); ++i;
    }
    auto out = pluss::codem::Series::empty({(std::int64_t)outv.size()}, pluss::codem::ScalarType::Int64);
    if (!outv.empty()) std::memcpy(out.raw_data(), outv.data(), outv.size() * sizeof(std::int64_t));
    return make(::pluss::codeh::Timeline(std::move(out), std::string(a.t_.tz())));
  }

  static ::pluss::codeh::TimeIndex tz_convert(const ::pluss::codeh::TimeIndex& self, std::string_view tz) {
    auto t = ::pluss::codeh::detail::impl_tz_convert(self.t_, tz);
    return make(std::move(t));
  }

  static ::pluss::codeh::TimeIndex tz_localize(const ::pluss::codeh::TimeIndex& self, std::string_view tz, int fold, std::string_view nonexistent) {
    auto t = ::pluss::codeh::detail::impl_tz_localize(self.t_, tz, fold, nonexistent);
    return make(std::move(t));
  }

  static ::pluss::codeh::TimeIndex round(const ::pluss::codeh::TimeIndex& self, std::string_view mode, std::string_view unit) {
    auto t = ::pluss::codeh::detail::impl_dt_round(self.t_, mode, unit);
    return make(std::move(t));
  }

  static pluss::codem::Series resample_agg(const ::pluss::codeh::TimeIndex& self,
                                          const pluss::codem::Series& values,
                                          std::string_view freq,
                                          std::string_view op,
                                          std::int64_t step) {
    return resample_agg(self, values, resample_options_(freq, step), op);
  }

  static pluss::codem::Series resample_agg(
      const ::pluss::codeh::TimeIndex& self,
      const pluss::codem::Series& values,
      const ::pluss::codeh::TimeIndex::ResampleOptions& options,
      std::string_view op) {
    if (op == "sum") return resample_sum(self, values, options);
    if (op == "mean") return resample_mean(self, values, options);
    if (op == "min") return resample_min(self, values, options);
    if (op == "max") return resample_max(self, values, options);
    if (op == "first") return resample_first(self, values, options);
    if (op == "last") return resample_last(self, values, options);
    if (op == "ohlc") return resample_ohlc(self, values, options);
    if (op == "count") {
      // ignore values; count observations
      return resample_count(self, options);
    }
    CODEM_FAIL("TimeIndex::resample_agg: unknown op");
  }

  static pluss::codem::Series rolling_agg(const ::pluss::codeh::TimeIndex& self,
                                         const pluss::codem::Series& values,
                                         const ::pluss::codeh::Duration& window,
                                         std::string_view op) {
    if (op == "sum") return rolling_sum(self, values, window);
    if (op == "count") return rolling_count(self, window);
    if (op == "mean") {
      auto s = rolling_sum(self, values, window);
      auto c = rolling_count(self, window);
      // Use CodeM div for numeric Series (routes through CodeM invoke).
      return s.div(c.to(pluss::codem::ScalarType::Float64));
    }
    // Simple O(n^2) min/max/first/last in UTC ticks; adequate baseline.
    check_values_1d_match_(values, self.size());
    CODEM_CHECK(values.scalar_type() == pluss::codem::ScalarType::Float64 || values.scalar_type() == pluss::codem::ScalarType::Int64,
                "rolling_agg: currently supports int64/float64 values");
    const auto n = self.size();
    const auto* t = reinterpret_cast<const std::int64_t*>(self.t_.ticks_.raw_data());
    const auto wflat = ::pluss::codeh::_compat::invoke<std::vector<std::int64_t>>("to_nanos_flat", window);
    CODEM_CHECK(wflat.size() == 1, "rolling_agg: window must be scalar Duration");
    const auto win = wflat[0];
    CODEM_CHECK(win != ::pluss::codeh::Duration::kNaT && win > 0, "rolling_agg: window must be positive");

    auto out = pluss::codem::Series::empty({n}, values.scalar_type());
    if (values.scalar_type() == pluss::codem::ScalarType::Float64) {
      const auto* v = reinterpret_cast<const double*>(values.raw_data());
      auto* o = reinterpret_cast<double*>(out.raw_data());
      for (std::int64_t i = 0; i < n; ++i) {
        const auto left = t[i] - win;
        double best = 0.0;
        bool any = false;
        double first = 0.0, last = 0.0;
        for (std::int64_t j = i; j >= 0; --j) {
          if (t[j] < left) break;
          if (!any) { best = v[j]; first = v[j]; any = true; }
          last = v[j];
          if (op == "min") best = std::min(best, v[j]);
          else if (op == "max") best = std::max(best, v[j]);
        }
        if (!any) { o[i] = std::numeric_limits<double>::quiet_NaN(); }
        else if (op == "first") o[i] = first;
        else if (op == "last") o[i] = last;
        else o[i] = best;
      }
    } else {
      const auto* v = reinterpret_cast<const std::int64_t*>(values.raw_data());
      auto* o = reinterpret_cast<std::int64_t*>(out.raw_data());
      for (std::int64_t i = 0; i < n; ++i) {
        const auto left = t[i] - win;
        std::int64_t best = 0;
        bool any = false;
        std::int64_t first = 0, last = 0;
        for (std::int64_t j = i; j >= 0; --j) {
          if (t[j] < left) break;
          if (!any) { best = v[j]; first = v[j]; any = true; }
          last = v[j];
          if (op == "min") best = std::min(best, v[j]);
          else if (op == "max") best = std::max(best, v[j]);
        }
        if (!any) { o[i] = 0; }
        else if (op == "first") o[i] = first;
        else if (op == "last") o[i] = last;
        else o[i] = best;
      }
    }
    return out;
  }

  static std::string infer_freq(const ::pluss::codeh::TimeIndex& self) {
    const auto n = self.size();
    if (n < 3) return std::string();
    const auto* x = reinterpret_cast<const std::int64_t*>(self.t_.ticks_.raw_data());
    std::int64_t best = 0;
    std::int64_t best_count = 0;
    // Count identical diffs.
    for (std::int64_t i = 1; i < n; ++i) {
      const auto d = x[i] - x[i-1];
      if (d <= 0) continue;
      std::int64_t cnt = 1;
      for (std::int64_t j = i+1; j < n; ++j) {
        if (x[j] - x[j-1] == d) ++cnt;
        else break;
      }
      if (cnt > best_count) { best_count = cnt; best = d; }
    }
    if (best == 0) return std::string();
    const std::int64_t s = 1'000'000'000LL;
    const std::int64_t m = 60LL * s;
    const std::int64_t h = 3600LL * s;
    const std::int64_t D = 86400LL * s;
    if (best % D == 0) return "D";
    if (best % h == 0) return "H";
    if (best % m == 0) return "min";
    if (best % s == 0) return "S";
    return std::string();
  }

  static pluss::coden::Note to_note_iso(const ::pluss::codeh::TimeIndex& self) {
    return ::pluss::codeh::_compat::invoke<pluss::coden::Note>("to_note_iso", self.t_);
  }

  static ::pluss::codeh::TimeIndex from_timeline(const ::pluss::codeh::Timeline& t, bool require_sorted) {
    CODEM_CHECK(t.defined(), "TimeIndex: timeline undefined");
    CODEM_CHECK(t.dim() == 1, "TimeIndex requires a 1D Timeline");

    // TimeIndex kernels use packed binary search and scans. Materialize a
    // strided Timeline once at the promotion boundary instead of letting each
    // ordered operation reinterpret non-contiguous storage as packed.
    auto ticks = t.ticks_.is_contiguous() ? t.ticks_ : t.ticks_.contiguous();
    const auto* x = reinterpret_cast<const std::int64_t*>(ticks.raw_data());
    for (std::int64_t i = 0; i < t.numel(); ++i) {
      CODEM_CHECK(x[i] != ::pluss::codeh::Timeline::kNaT,
                  "TimeIndex does not allow NaT");
    }
    if (require_sorted) {
      for (std::int64_t i = 1; i < t.numel(); ++i) {
        CODEM_CHECK(x[i - 1] <= x[i], "TimeIndex must be sorted (non-decreasing)");
      }
    }
    return make(::pluss::codeh::Timeline(std::move(ticks), std::string(t.tz())));
  }

  static std::int64_t searchsorted(const ::pluss::codeh::TimeIndex& self, const ::pluss::codeh::Timeline& key, bool right) {
    CODEM_CHECK(key.dim() == 0, "TimeIndex::searchsorted expects scalar key");
    const auto k = reinterpret_cast<const std::int64_t*>(key.ticks_.raw_data())[0];
    CODEM_CHECK(k != ::pluss::codeh::Timeline::kNaT, "searchsorted: key cannot be NaT");
    const auto* x = reinterpret_cast<const std::int64_t*>(self.t_.ticks_.raw_data());
    const auto n = self.size();
    if (!right) return (std::int64_t)(std::lower_bound(x, x + n, k) - x);
    return (std::int64_t)(std::upper_bound(x, x + n, k) - x);
  }

  static std::int64_t asof(const ::pluss::codeh::TimeIndex& self, const ::pluss::codeh::Timeline& key) {
    CODEM_CHECK(key.dim() == 0, "TimeIndex::asof expects scalar key");
    const auto k = reinterpret_cast<const std::int64_t*>(key.ticks_.raw_data())[0];
    CODEM_CHECK(k != ::pluss::codeh::Timeline::kNaT, "asof: key cannot be NaT");
    const auto* x = reinterpret_cast<const std::int64_t*>(self.t_.ticks_.raw_data());
    const auto n = self.size();
    auto it = std::upper_bound(x, x + n, k);
    if (it == x) return -1;
    return (std::int64_t)((it - x) - 1);
  }

  static pluss::codem::Series slice(const ::pluss::codeh::TimeIndex& self, const ::pluss::codeh::Timeline& start, const ::pluss::codeh::Timeline& end) {
    CODEM_CHECK(start.dim() == 0 && end.dim() == 0, "TimeIndex::slice expects scalar bounds");
    const auto s = reinterpret_cast<const std::int64_t*>(start.ticks_.raw_data())[0];
    const auto e = reinterpret_cast<const std::int64_t*>(end.ticks_.raw_data())[0];
    CODEM_CHECK(s != ::pluss::codeh::Timeline::kNaT && e != ::pluss::codeh::Timeline::kNaT, "slice: bounds cannot be NaT");
    const auto* x = reinterpret_cast<const std::int64_t*>(self.t_.ticks_.raw_data());
    const auto n = self.size();
    const auto lo = (std::int64_t)(std::lower_bound(x, x + n, s) - x);
    const auto hi = (std::int64_t)(std::upper_bound(x, x + n, e) - x);
    const std::int64_t len = std::max<std::int64_t>(0, hi - lo);
    auto out = pluss::codem::Series::empty({len}, pluss::codem::ScalarType::Int64);
    auto* o = reinterpret_cast<std::int64_t*>(out.raw_data());
    for (std::int64_t i = 0; i < len; ++i) o[i] = lo + i;
    return out;
  }

  static pluss::codem::Series slice_time(const ::pluss::codeh::TimeIndex& self, std::string_view start_iso, std::string_view end_iso) {
    const auto s = parse_scalar_iso_(start_iso);
    const auto e = parse_scalar_iso_(end_iso);
    return slice(self, s, e);
  }

  static ::pluss::codeh::TimeIndex::ResampleBins resample_bins(
      const ::pluss::codeh::TimeIndex& self,
      const ::pluss::codeh::TimeIndex::ResampleOptions& options) {
    using namespace std::chrono;
    const auto parsed = parse_resample_freq_(options.freq, options.step);
    const auto fixed_span = parsed.unit_nanos * parsed.multiplier;
    const auto* x = reinterpret_cast<const std::int64_t*>(self.t_.ticks_.raw_data());
    const auto n = self.size();
    if (n == 0) {
      auto labels = ::pluss::codeh::Timeline(pluss::codem::Series::empty({0}, pluss::codem::ScalarType::Int64), std::string(self.t_.tz()));
      auto bin = pluss::codem::Series::empty({0}, pluss::codem::ScalarType::Int64);
      return {std::move(labels), std::move(bin)};
    }
    ::pluss::codeh::TimeZone tz = ::pluss::codeh::TimeZone::load(self.t_.tz());
    std::int64_t min_local = std::numeric_limits<std::int64_t>::max();
    for (std::int64_t i = 0; i < n; ++i) {
      const auto utc = x[i];
      CODEM_CHECK(utc != ::pluss::codeh::Timeline::kNaT, "TimeIndex does not allow NaT");
      const int off = tz.offset_minutes_at_utc(utc);
      const std::int64_t local = utc + (std::int64_t)off * 60LL * 1'000'000'000LL;
      min_local = std::min(min_local, local);
    }
    auto bin_id = pluss::codem::Series::empty({n}, pluss::codem::ScalarType::Int64);
    auto* bid = reinterpret_cast<std::int64_t*>(bin_id.raw_data());
    std::int64_t max_bin = std::numeric_limits<std::int64_t>::min();
    std::int64_t min_bin = std::numeric_limits<std::int64_t>::max();

    constexpr std::int64_t kDayNanos = 86'400LL * 1'000'000'000LL;
    const auto to_local_ymd_ = [](std::int64_t local_ns) {
      const sys_time<nanoseconds> tp{nanoseconds(local_ns)};
      const auto dp = floor<days>(tp);
      const auto ymd = year_month_day(dp);
      return std::tuple<int, unsigned, unsigned, std::int64_t>(
          static_cast<int>(ymd.year()),
          static_cast<unsigned>(ymd.month()),
          static_cast<unsigned>(ymd.day()),
          duration_cast<nanoseconds>(dp.time_since_epoch()).count());
    };

    std::int64_t anchor_local = 0;
    int anchor_year = 1970;
    unsigned anchor_month = 1;
    if (parsed.kind == ParsedResampleFreq::Kind::Fixed) {
      if (options.origin == ::pluss::codeh::TimeIndex::ResampleOrigin::Start) {
        anchor_local = min_local;
      } else if (options.origin == ::pluss::codeh::TimeIndex::ResampleOrigin::Epoch) {
        anchor_local = 0;
      } else {
        anchor_local = floor_div_i64_(min_local, kDayNanos) * kDayNanos;
      }
      anchor_local += options.offset_nanos;
    } else if (parsed.kind == ParsedResampleFreq::Kind::Week) {
      const auto [yy, mm, dd, day_floor] = to_local_ymd_(min_local);
      (void)yy;
      (void)mm;
      (void)dd;
      if (options.origin == ::pluss::codeh::TimeIndex::ResampleOrigin::Start) {
        anchor_local = min_local;
      } else if (options.origin == ::pluss::codeh::TimeIndex::ResampleOrigin::Epoch) {
        anchor_local = 0;
      } else {
        const sys_time<nanoseconds> tp{nanoseconds(day_floor)};
        const auto dp = floor<days>(tp);
        const int wd_sun0 = static_cast<int>(weekday(dp).c_encoding());
        anchor_local = day_floor - static_cast<std::int64_t>(wd_sun0) * kDayNanos;
      }
      anchor_local += options.offset_nanos;
    } else {
      const auto [yy, mm, dd, day_floor] = to_local_ymd_(min_local);
      (void)dd;
      (void)day_floor;
      if (options.origin == ::pluss::codeh::TimeIndex::ResampleOrigin::Epoch) {
        anchor_year = 1970;
        anchor_month = 1;
      } else {
        anchor_year = yy;
        anchor_month = options.origin == ::pluss::codeh::TimeIndex::ResampleOrigin::Start ? mm : 1;
      }
    }

    const auto month_edge_local_ = [](int anchor_year_value,
                                      unsigned anchor_month_value,
                                      std::int64_t step_index) {
      const auto month_index =
          static_cast<std::int64_t>(anchor_month_value - 1) + step_index;
      const auto yy = anchor_year_value + static_cast<int>(floor_div_i64_(month_index, 12LL));
      auto mm0 = month_index % 12LL;
      if (mm0 < 0) mm0 += 12LL;
      const auto mm = static_cast<unsigned>(mm0) + 1U;
      const auto dp = sys_days(year(yy) / month(mm) / day(1));
      return duration_cast<nanoseconds>(dp.time_since_epoch()).count();
    };
    const auto year_edge_local_ = [](int anchor_year_value, std::int64_t step_index) {
      const auto yy = anchor_year_value + static_cast<int>(step_index);
      const auto dp = sys_days(year(yy) / month(1) / day(1));
      return duration_cast<nanoseconds>(dp.time_since_epoch()).count();
    };

    for (std::int64_t i = 0; i < n; ++i) {
      const auto utc = x[i];
      CODEM_CHECK(utc != ::pluss::codeh::Timeline::kNaT, "TimeIndex does not allow NaT");
      const int off = tz.offset_minutes_at_utc(utc);
      const std::int64_t local = utc + (std::int64_t)off * 60LL * 1'000'000'000LL;
      std::int64_t b = 0;
      if (parsed.kind == ParsedResampleFreq::Kind::Fixed || parsed.kind == ParsedResampleFreq::Kind::Week) {
        const auto shifted = local - anchor_local;
        const auto adjusted =
            options.closed == ::pluss::codeh::TimeIndex::ResampleClosed::Right && shifted != 0
                ? shifted - 1
                : shifted;
        b = floor_div_i64_(adjusted, fixed_span);
      } else {
        const auto [yy, mm, dd, day_floor] = to_local_ymd_(local);
        (void)dd;
        (void)day_floor;
        if (parsed.kind == ParsedResampleFreq::Kind::Month) {
          const auto month_index = static_cast<std::int64_t>(yy - anchor_year) * 12LL
                                 + (static_cast<std::int64_t>(mm) - static_cast<std::int64_t>(anchor_month));
          b = floor_div_i64_(month_index, parsed.multiplier);
          if (options.closed == ::pluss::codeh::TimeIndex::ResampleClosed::Right &&
              month_index != 0 &&
              month_index % parsed.multiplier == 0 &&
              local == month_edge_local_(anchor_year, anchor_month, month_index)) {
            --b;
          }
        } else {
          const auto year_index = static_cast<std::int64_t>(yy - anchor_year);
          b = floor_div_i64_(year_index, parsed.multiplier);
          if (options.closed == ::pluss::codeh::TimeIndex::ResampleClosed::Right &&
              year_index != 0 &&
              year_index % parsed.multiplier == 0 &&
              local == year_edge_local_(anchor_year, year_index)) {
            --b;
          }
        }
      }
      bid[i] = b;
      if (b > max_bin) max_bin = b;
      if (b < min_bin) min_bin = b;
    }

    if (min_bin != 0) {
      for (std::int64_t i = 0; i < n; ++i) {
        bid[i] -= min_bin;
      }
    }

    const std::int64_t nbins = max_bin - min_bin + 1;
    auto label_ticks = pluss::codem::Series::empty({nbins}, pluss::codem::ScalarType::Int64);
    auto* lt = reinterpret_cast<std::int64_t*>(label_ticks.raw_data());
    for (std::int64_t b = 0; b < nbins; ++b) {
      const auto logical_bin = b + min_bin;
      const auto label_bin =
          options.label == ::pluss::codeh::TimeIndex::ResampleLabel::Right ? logical_bin + 1 : logical_bin;
      std::int64_t local = 0;
      if (parsed.kind == ParsedResampleFreq::Kind::Fixed || parsed.kind == ParsedResampleFreq::Kind::Week) {
        local = anchor_local + label_bin * fixed_span;
      } else if (parsed.kind == ParsedResampleFreq::Kind::Month) {
        local = month_edge_local_(anchor_year, anchor_month, label_bin * parsed.multiplier);
      } else {
        local = year_edge_local_(anchor_year, label_bin * parsed.multiplier);
      }
      const std::int64_t utc = tz.local_to_utc(local, /*fold=*/0, /*nonexistent=*/"shift_forward");
      lt[b] = utc;
    }
    ::pluss::codeh::Timeline labels(std::move(label_ticks), std::string(self.t_.tz()));
    return {std::move(labels), std::move(bin_id)};
  }

  static ::pluss::codeh::TimeIndex::ResampleBins resample_bins(
      const ::pluss::codeh::TimeIndex& self,
      std::string_view freq,
      std::int64_t step) {
    return resample_bins(self, resample_options_(freq, step));
  }

  static ::pluss::codeh::TimeIndex::ResampleBins resample_bins(
      const ::pluss::codeh::TimeIndex& self,
      std::string_view freq,
      std::int64_t step,
      ::pluss::codeh::TimeIndex::ResampleClosed closed,
      ::pluss::codeh::TimeIndex::ResampleLabel label,
      ::pluss::codeh::TimeIndex::ResampleOrigin origin,
      std::int64_t offset_nanos) {
    return resample_bins(self, resample_options_(freq, step, closed, label, origin, offset_nanos));
  }

  static pluss::codem::Series resample_count(const ::pluss::codeh::TimeIndex& self, std::string_view freq, std::int64_t step) {
    return resample_count(self, resample_options_(freq, step));
  }

  static pluss::codem::Series resample_count(
      const ::pluss::codeh::TimeIndex& self,
      const ::pluss::codeh::TimeIndex::ResampleOptions& options) {
    auto bins = resample_bins(self, options);
    const auto n = self.size();
    const auto nb = bins.labels.numel();
    auto out = pluss::codem::Series::empty({nb}, pluss::codem::ScalarType::Int64);
    auto* o = reinterpret_cast<std::int64_t*>(out.raw_data());
    std::fill(o, o + nb, 0);
    const auto* bid = reinterpret_cast<const std::int64_t*>(bins.bin_id.raw_data());
    for (std::int64_t i = 0; i < n; ++i) {
      const auto b = bid[i];
      if (b >= 0 && b < nb) o[b] += 1;
    }
    return out;
  }

  template <class T>
  static pluss::codem::Series resample_minmax_impl_(const pluss::codem::Series& values,
                                                   const pluss::codem::Series& bin_id,
                                                   std::int64_t nbins,
                                                   bool is_min) {
    auto out = pluss::codem::Series::empty({nbins}, values.scalar_type());
    auto o = out.contiguous();
    auto v = values.contiguous();
    auto* op = reinterpret_cast<T*>(o.raw_data());
    const auto* vp = reinterpret_cast<const T*>(v.raw_data());
    const auto* bid = reinterpret_cast<const std::int64_t*>(bin_id.raw_data());
    std::vector<std::uint8_t> seen((std::size_t)nbins, 0);
    for (std::int64_t i = 0; i < v.numel(); ++i) {
      const auto b = bid[i];
      if (!seen[(std::size_t)b]) {
        op[b] = vp[i];
        seen[(std::size_t)b] = 1;
      } else {
        if (is_min) op[b] = (vp[i] < op[b]) ? vp[i] : op[b];
        else op[b] = (vp[i] > op[b]) ? vp[i] : op[b];
      }
    }
    for (std::int64_t b = 0; b < nbins; ++b) {
      if (!seen[(std::size_t)b]) {
        if constexpr (std::is_floating_point_v<T>) op[b] = std::numeric_limits<T>::quiet_NaN();
        else op[b] = T{};
      }
    }
    return out;
  }

  static pluss::codem::Series resample_sum(const ::pluss::codeh::TimeIndex& self, const pluss::codem::Series& values, std::string_view freq, std::int64_t step) {
    return resample_sum(self, values, resample_options_(freq, step));
  }

  static pluss::codem::Series resample_sum(
      const ::pluss::codeh::TimeIndex& self,
      const pluss::codem::Series& values,
      const ::pluss::codeh::TimeIndex::ResampleOptions& options) {
    check_values_1d_match_(values, self.size());
    auto bins = resample_bins(self, options);
    const auto nb = bins.labels.numel();
    auto out = pluss::codem::Series::empty({nb}, values.scalar_type());
    auto ocont = out.contiguous();
    std::memset(ocont.raw_data(), 0,
                (std::size_t)ocont.numel() * (std::size_t)ocont.itemsize());
    auto vcont = values.contiguous();
    const auto* bid = reinterpret_cast<const std::int64_t*>(bins.bin_id.raw_data());

    switch (values.scalar_type()) {
      case pluss::codem::ScalarType::Int64: {
        auto* o = reinterpret_cast<std::int64_t*>(ocont.raw_data());
        const auto* v = reinterpret_cast<const std::int64_t*>(vcont.raw_data());
        for (std::int64_t i = 0; i < vcont.numel(); ++i) o[bid[i]] += v[i];
        break;
      }
      case pluss::codem::ScalarType::Float64: {
        auto* o = reinterpret_cast<double*>(ocont.raw_data());
        const auto* v = reinterpret_cast<const double*>(vcont.raw_data());
        for (std::int64_t i = 0; i < vcont.numel(); ++i) o[bid[i]] += v[i];
        break;
      }
      case pluss::codem::ScalarType::Float32: {
        auto* o = reinterpret_cast<float*>(ocont.raw_data());
        const auto* v = reinterpret_cast<const float*>(vcont.raw_data());
        for (std::int64_t i = 0; i < vcont.numel(); ++i) o[bid[i]] += v[i];
        break;
      }
      default:
        return resample_sum(self, values.to(pluss::codem::ScalarType::Float64), options).to(values.scalar_type());
    }
    return out;
  }

  static pluss::codem::Series resample_mean(const ::pluss::codeh::TimeIndex& self, const pluss::codem::Series& values, std::string_view freq, std::int64_t step) {
    return resample_mean(self, values, resample_options_(freq, step));
  }

  static pluss::codem::Series resample_mean(
      const ::pluss::codeh::TimeIndex& self,
      const pluss::codem::Series& values,
      const ::pluss::codeh::TimeIndex::ResampleOptions& options) {
    check_values_1d_match_(values, self.size());
    auto bins = resample_bins(self, options);
    const auto nb = bins.labels.numel();
    auto sum = resample_sum(self, values.to(pluss::codem::ScalarType::Float64), options);
    auto cnt = resample_count(self, options);
    auto out = pluss::codem::Series::empty({nb}, pluss::codem::ScalarType::Float64);
    auto* o = reinterpret_cast<double*>(out.raw_data());
    const auto* s = reinterpret_cast<const double*>(sum.raw_data());
    const auto* c = reinterpret_cast<const std::int64_t*>(cnt.raw_data());
    for (std::int64_t b = 0; b < nb; ++b) o[b] = (c[b] == 0) ? std::numeric_limits<double>::quiet_NaN() : (s[b] / (double)c[b]);
    return out;
  }

  static pluss::codem::Series resample_min(const ::pluss::codeh::TimeIndex& self, const pluss::codem::Series& values, std::string_view freq, std::int64_t step) {
    return resample_min(self, values, resample_options_(freq, step));
  }

  static pluss::codem::Series resample_min(
      const ::pluss::codeh::TimeIndex& self,
      const pluss::codem::Series& values,
      const ::pluss::codeh::TimeIndex::ResampleOptions& options) {
    check_values_1d_match_(values, self.size());
    auto bins = resample_bins(self, options);
    const auto nb = bins.labels.numel();
    switch (values.scalar_type()) {
      case pluss::codem::ScalarType::Int64: return resample_minmax_impl_<std::int64_t>(values, bins.bin_id, nb, true);
      case pluss::codem::ScalarType::Float64: return resample_minmax_impl_<double>(values, bins.bin_id, nb, true);
      case pluss::codem::ScalarType::Float32: return resample_minmax_impl_<float>(values, bins.bin_id, nb, true);
      default: return resample_min(self, values.to(pluss::codem::ScalarType::Float64), options).to(values.scalar_type());
    }
  }

  static pluss::codem::Series resample_max(const ::pluss::codeh::TimeIndex& self, const pluss::codem::Series& values, std::string_view freq, std::int64_t step) {
    return resample_max(self, values, resample_options_(freq, step));
  }

  static pluss::codem::Series resample_max(
      const ::pluss::codeh::TimeIndex& self,
      const pluss::codem::Series& values,
      const ::pluss::codeh::TimeIndex::ResampleOptions& options) {
    check_values_1d_match_(values, self.size());
    auto bins = resample_bins(self, options);
    const auto nb = bins.labels.numel();
    switch (values.scalar_type()) {
      case pluss::codem::ScalarType::Int64: return resample_minmax_impl_<std::int64_t>(values, bins.bin_id, nb, false);
      case pluss::codem::ScalarType::Float64: return resample_minmax_impl_<double>(values, bins.bin_id, nb, false);
      case pluss::codem::ScalarType::Float32: return resample_minmax_impl_<float>(values, bins.bin_id, nb, false);
      default: return resample_max(self, values.to(pluss::codem::ScalarType::Float64), options).to(values.scalar_type());
    }
  }

  static pluss::codem::Series resample_first_last(const ::pluss::codeh::TimeIndex& self, const pluss::codem::Series& values, std::string_view freq, std::int64_t step, bool is_first) {
    return resample_first_last(self, values, resample_options_(freq, step), is_first);
  }

  static pluss::codem::Series resample_first_last(
      const ::pluss::codeh::TimeIndex& self,
      const pluss::codem::Series& values,
      const ::pluss::codeh::TimeIndex::ResampleOptions& options,
      bool is_first) {
    check_values_1d_match_(values, self.size());
    auto bins = resample_bins(self, options);
    const auto nb = bins.labels.numel();
    auto out = pluss::codem::Series::empty({nb}, values.scalar_type());
    auto o = out.contiguous();
    auto v = values.contiguous();
    const auto* bid = reinterpret_cast<const std::int64_t*>(bins.bin_id.raw_data());
    std::vector<std::uint8_t> seen((std::size_t)nb, 0);
    // first: forward fill; last: overwrite
    for (std::int64_t i = 0; i < v.numel(); ++i) {
      const auto b = bid[i];
      if (is_first) {
        if (seen[(std::size_t)b]) continue;
        std::memcpy((char*)o.raw_data() + (std::size_t)b * (std::size_t)o.itemsize(),
                    (const char*)v.raw_data() + (std::size_t)i * (std::size_t)v.itemsize(),
                    (std::size_t)o.itemsize());
        seen[(std::size_t)b] = 1;
      } else {
        std::memcpy((char*)o.raw_data() + (std::size_t)b * (std::size_t)o.itemsize(),
                    (const char*)v.raw_data() + (std::size_t)i * (std::size_t)v.itemsize(),
                    (std::size_t)o.itemsize());
        seen[(std::size_t)b] = 1;
      }
    }
    // fill empties
    for (std::int64_t b = 0; b < nb; ++b) {
      if (!seen[(std::size_t)b]) {
        if (o.scalar_type() == pluss::codem::ScalarType::Float32) reinterpret_cast<float*>(o.raw_data())[b] = std::numeric_limits<float>::quiet_NaN();
        else if (o.scalar_type() == pluss::codem::ScalarType::Float64) reinterpret_cast<double*>(o.raw_data())[b] = std::numeric_limits<double>::quiet_NaN();
        else if (o.scalar_type() == pluss::codem::ScalarType::Int64) reinterpret_cast<std::int64_t*>(o.raw_data())[b] = 0;
        else {
          // best-effort: zero bytes
          std::memset((char*)o.raw_data() + (std::size_t)b * (std::size_t)o.itemsize(), 0, (std::size_t)o.itemsize());
        }
      }
    }
    return out;
  }

  static pluss::codem::Series resample_first(const ::pluss::codeh::TimeIndex& self, const pluss::codem::Series& values, std::string_view freq, std::int64_t step) {
    return resample_first_last(self, values, freq, step, true);
  }
  static pluss::codem::Series resample_first(
      const ::pluss::codeh::TimeIndex& self,
      const pluss::codem::Series& values,
      const ::pluss::codeh::TimeIndex::ResampleOptions& options) {
    return resample_first_last(self, values, options, true);
  }
  static pluss::codem::Series resample_last(const ::pluss::codeh::TimeIndex& self, const pluss::codem::Series& values, std::string_view freq, std::int64_t step) {
    return resample_first_last(self, values, freq, step, false);
  }
  static pluss::codem::Series resample_last(
      const ::pluss::codeh::TimeIndex& self,
      const pluss::codem::Series& values,
      const ::pluss::codeh::TimeIndex::ResampleOptions& options) {
    return resample_first_last(self, values, options, false);
  }

  static pluss::codem::Series resample_ohlc(const ::pluss::codeh::TimeIndex& self, const pluss::codem::Series& values, std::string_view freq, std::int64_t step) {
    return resample_ohlc(self, values, resample_options_(freq, step));
  }

  static pluss::codem::Series resample_ohlc(
      const ::pluss::codeh::TimeIndex& self,
      const pluss::codem::Series& values,
      const ::pluss::codeh::TimeIndex::ResampleOptions& options) {
    check_values_1d_match_(values, self.size());
    auto bins = resample_bins(self, options);
    const auto nb = bins.labels.numel();
    auto out = pluss::codem::Series::empty({nb, 4}, values.scalar_type());
    auto o = out.contiguous();
    // open/close via first/last; high/low via max/min
    auto open = resample_first(self, values, options).contiguous();
    auto high = resample_max(self, values, options).contiguous();
    auto low  = resample_min(self, values, options).contiguous();
    auto close= resample_last(self, values, options).contiguous();
    for (std::int64_t b = 0; b < nb; ++b) {
      std::memcpy((char*)o.raw_data() + (std::size_t)(b*4+0) * (std::size_t)o.itemsize(), (const char*)open.raw_data() + (std::size_t)b * (std::size_t)o.itemsize(), (std::size_t)o.itemsize());
      std::memcpy((char*)o.raw_data() + (std::size_t)(b*4+1) * (std::size_t)o.itemsize(), (const char*)high.raw_data() + (std::size_t)b * (std::size_t)o.itemsize(), (std::size_t)o.itemsize());
      std::memcpy((char*)o.raw_data() + (std::size_t)(b*4+2) * (std::size_t)o.itemsize(), (const char*)low.raw_data() + (std::size_t)b * (std::size_t)o.itemsize(), (std::size_t)o.itemsize());
      std::memcpy((char*)o.raw_data() + (std::size_t)(b*4+3) * (std::size_t)o.itemsize(), (const char*)close.raw_data() + (std::size_t)b * (std::size_t)o.itemsize(), (std::size_t)o.itemsize());
    }
    return out;
  }

  static pluss::codem::Series rolling_count(const ::pluss::codeh::TimeIndex& self, const ::pluss::codeh::Duration& window) {
    CODEM_CHECK(window.dim() == 0, "rolling_count expects scalar window");
    const auto w = reinterpret_cast<const std::int64_t*>(window.nanos_.raw_data())[0];
    CODEM_CHECK(w > 0, "rolling_count: window must be positive");
    const auto n = self.size();
    auto out = pluss::codem::Series::empty({n}, pluss::codem::ScalarType::Int64);
    auto* o = reinterpret_cast<std::int64_t*>(out.raw_data());
    const auto* x = reinterpret_cast<const std::int64_t*>(self.t_.ticks_.raw_data());
    std::int64_t j = 0;
    for (std::int64_t i = 0; i < n; ++i) {
      const auto hi = x[i];
      const auto lo = hi - w;
      while (j < i && x[j] < lo) ++j;
      o[i] = i - j + 1;
    }
    return out;
  }

  static pluss::codem::Series rolling_sum(const ::pluss::codeh::TimeIndex& self, const pluss::codem::Series& values, const ::pluss::codeh::Duration& window) {
    check_values_1d_match_(values, self.size());
    CODEM_CHECK(window.dim() == 0, "rolling_sum expects scalar window");
    const auto w = reinterpret_cast<const std::int64_t*>(window.nanos_.raw_data())[0];
    CODEM_CHECK(w > 0, "rolling_sum: window must be positive");
    const auto n = self.size();
    auto out = pluss::codem::Series::empty({n}, values.scalar_type());

    // Minimal implementation: recompute sums with two pointers for float64/int64.
    const auto* x = reinterpret_cast<const std::int64_t*>(self.t_.ticks_.raw_data());
    if (values.scalar_type() == pluss::codem::ScalarType::Float64) {
      const auto* v = reinterpret_cast<const double*>(values.contiguous().raw_data());
      auto* o = reinterpret_cast<double*>(out.raw_data());
      std::int64_t j = 0;
      double acc = 0.0;
      for (std::int64_t i = 0; i < n; ++i) {
        const auto hi = x[i];
        const auto lo = hi - w;
        acc += v[i];
        while (j < i && x[j] < lo) { acc -= v[j]; ++j; }
        o[i] = acc;
      }
      return out;
    }
    if (values.scalar_type() == pluss::codem::ScalarType::Int64) {
      const auto* v = reinterpret_cast<const std::int64_t*>(values.contiguous().raw_data());
      auto* o = reinterpret_cast<std::int64_t*>(out.raw_data());
      std::int64_t j = 0;
      std::int64_t acc = 0;
      for (std::int64_t i = 0; i < n; ++i) {
        const auto hi = x[i];
        const auto lo = hi - w;
        acc += v[i];
        while (j < i && x[j] < lo) { acc -= v[j]; ++j; }
        o[i] = acc;
      }
      return out;
    }
    // Fallback promote
    return rolling_sum(self, values.to(pluss::codem::ScalarType::Float64), window).to(values.scalar_type());
  }

  static pluss::codem::Series join_asof(const ::pluss::codeh::TimeIndex& left,
                                       const ::pluss::codeh::TimeIndex& right,
                                       ::pluss::codeh::TimeIndex::AsofDirection direction,
                                       const ::pluss::codeh::Duration* tolerance,
                                       bool allow_exact_matches) {
    const auto nl = left.size();
    const auto nr = right.size();
    auto out = pluss::codem::Series::empty({nl}, pluss::codem::ScalarType::Int64);
    auto* o = reinterpret_cast<std::int64_t*>(out.raw_data());
    const auto* xl = reinterpret_cast<const std::int64_t*>(left.t_.ticks_.raw_data());
    const auto* xr = reinterpret_cast<const std::int64_t*>(right.t_.ticks_.raw_data());
    const std::int64_t tol = tolerance ? reinterpret_cast<const std::int64_t*>(tolerance->nanos_.raw_data())[0] : -1;

    std::int64_t j = 0;
    for (std::int64_t i = 0; i < nl; ++i) {
      const auto k = xl[i];
      if (direction == ::pluss::codeh::TimeIndex::AsofDirection::Backward) {
        while (j < nr && xr[j] <= k) ++j;
        std::int64_t idx = j - 1;
        if (!allow_exact_matches && idx >= 0 && xr[idx] == k) --idx;
        if (idx < 0) { o[i] = -1; continue; }
        if (tol >= 0 && (k - xr[idx]) > tol) { o[i] = -1; continue; }
        o[i] = idx;
      } else if (direction == ::pluss::codeh::TimeIndex::AsofDirection::Forward) {
        while (j < nr && xr[j] < k) ++j;
        std::int64_t idx = j;
        if (!allow_exact_matches && idx < nr && xr[idx] == k) ++idx;
        if (idx >= nr) { o[i] = -1; continue; }
        if (tol >= 0 && (xr[idx] - k) > tol) { o[i] = -1; continue; }
        o[i] = idx;
      } else { // Nearest
        while (j < nr && xr[j] < k) ++j;
        std::int64_t cand1 = j - 1;
        std::int64_t cand2 = j;
        std::int64_t best = -1;
        std::int64_t bestd = (std::numeric_limits<std::int64_t>::max)();
        auto consider = [&](std::int64_t idx) {
          if (idx < 0 || idx >= nr) return;
          if (!allow_exact_matches && xr[idx] == k) return;
          const std::int64_t d = (xr[idx] > k) ? (xr[idx] - k) : (k - xr[idx]);
          if (d < bestd) { bestd = d; best = idx; }
        };
        consider(cand1);
        consider(cand2);
        if (best < 0) { o[i] = -1; continue; }
        if (tol >= 0 && bestd > tol) { o[i] = -1; continue; }
        o[i] = best;
      }
    }
    return out;
  }

  static ::pluss::codeh::TimeIndex::AlignUnion align_union(const ::pluss::codeh::TimeIndex& left,
                                                          const ::pluss::codeh::TimeIndex& right) {
    const auto nl = left.size();
    const auto nr = right.size();
    const auto* xl = reinterpret_cast<const std::int64_t*>(left.t_.ticks_.raw_data());
    const auto* xr = reinterpret_cast<const std::int64_t*>(right.t_.ticks_.raw_data());

    std::vector<std::int64_t> u;
    u.reserve((std::size_t)(nl + nr));
    std::vector<std::int64_t> li;
    std::vector<std::int64_t> ri;
    li.reserve((std::size_t)(nl + nr));
    ri.reserve((std::size_t)(nl + nr));

    std::int64_t i = 0, j = 0;
    while (i < nl || j < nr) {
      const bool take_l = (j >= nr) || (i < nl && xl[i] <= xr[j]);
      const bool take_r = (i >= nl) || (j < nr && xr[j] <= xl[i]);
      if (take_l && take_r && i < nl && j < nr && xl[i] == xr[j]) {
        u.push_back(xl[i]);
        li.push_back(i);
        ri.push_back(j);
        ++i; ++j;
      } else if (take_l) {
        u.push_back(xl[i]);
        li.push_back(i);
        ri.push_back(-1);
        ++i;
      } else {
        u.push_back(xr[j]);
        li.push_back(-1);
        ri.push_back(j);
        ++j;
      }
    }

    auto uticks = pluss::codem::Series::empty({(std::int64_t)u.size()}, pluss::codem::ScalarType::Int64);
    std::memcpy(uticks.raw_data(), u.data(), u.size() * sizeof(std::int64_t));
    ::pluss::codeh::Timeline ut(std::move(uticks), std::string(left.t_.tz()));

    auto lser = pluss::codem::Series::empty({(std::int64_t)li.size()}, pluss::codem::ScalarType::Int64);
    auto rser = pluss::codem::Series::empty({(std::int64_t)ri.size()}, pluss::codem::ScalarType::Int64);
    std::memcpy(lser.raw_data(), li.data(), li.size() * sizeof(std::int64_t));
    std::memcpy(rser.raw_data(), ri.data(), ri.size() * sizeof(std::int64_t));
    return {std::move(ut), std::move(lser), std::move(rser)};
  }
};

} // namespace pluss::codeh::_compat::detail
