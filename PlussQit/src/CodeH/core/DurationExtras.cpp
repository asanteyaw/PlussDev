#include "CodeH/core/Duration.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <string>

namespace pluss::codeh {

static void check_duration_core_(const pluss::codem::Series& s) {
  CODEM_CHECK(s.defined(), "Duration: underlying Series is undefined");
  CODEM_CHECK(s.scalar_type() == pluss::codem::ScalarType::Int64, "Duration: nanos must be int64");
  CODEM_CHECK(s.dim() >= 0 && s.dim() <= 3, "Duration: only supports 0D..3D");
}

static std::vector<std::int64_t> broadcast_simple_(std::span<const std::int64_t> a,
                                                  std::span<const std::int64_t> b) {
  if (a.size() == 0) return std::vector<std::int64_t>(b.begin(), b.end());
  if (b.size() == 0) return std::vector<std::int64_t>(a.begin(), a.end());
  CODEM_CHECK(a.size() == b.size(), "Duration: shape mismatch (dims differ)");
  for (size_t i = 0; i < a.size(); ++i) CODEM_CHECK(a[i] == b[i], "Duration: shape mismatch");
  return std::vector<std::int64_t>(a.begin(), a.end());
}

static std::int64_t unit_nanos_(std::string_view u) {
  if (u == "ns") return 1;
  if (u == "us") return 1'000LL;
  if (u == "ms") return 1'000'000LL;
  if (u == "s" || u == "sec" || u == "S") return 1'000'000'000LL;
  if (u == "min" || u == "m") return 60LL * 1'000'000'000LL;
  if (u == "h" || u == "H") return 3600LL * 1'000'000'000LL;
  if (u == "D" || u == "d") return 86400LL * 1'000'000'000LL;
  if (u == "W" || u == "w") return 7LL * 86400LL * 1'000'000'000LL;
  CODEM_FAIL("Duration: unsupported unit");
}

namespace detail {

pluss::codem::Series impl_cmp_duration(const Duration& a, const Duration& b, std::string_view op) {
  check_duration_core_(a.nanos_);
  check_duration_core_(b.nanos_);
  const auto out_sizes = broadcast_simple_(a.nanos_.sizes(), b.nanos_.sizes());
  auto out = pluss::codem::Series::empty(out_sizes, pluss::codem::ScalarType::Bool);

  auto it = pluss::codem::SeriesIterator::binary(a.nanos_, b.nanos_, out);
  const auto* av = reinterpret_cast<const std::int64_t*>(a.nanos_.raw_data());
  const auto* bv = reinterpret_cast<const std::int64_t*>(b.nanos_.raw_data());
  auto* o = reinterpret_cast<std::uint8_t*>(out.raw_data());

  it.for_each_index([&](std::int64_t, std::int64_t ao, std::int64_t bo, std::int64_t oo) {
    const auto x = av[ao];
    const auto y = bv[bo];
    bool r = false;
    if (x == Duration::kNaT || y == Duration::kNaT) {
      r = false;
    } else if (op == "eq") r = (x == y);
    else if (op == "ne") r = (x != y);
    else if (op == "lt") r = (x < y);
    else if (op == "le") r = (x <= y);
    else if (op == "gt") r = (x > y);
    else if (op == "ge") r = (x >= y);
    else CODEM_FAIL("Duration cmp: unknown op");
    o[oo] = r ? 1 : 0;
  });
  return out;
}

pluss::codem::Series impl_isnat_duration(const Duration& a) {
  check_duration_core_(a.nanos_);
  auto out = pluss::codem::Series::empty(a.nanos_.sizes(), pluss::codem::ScalarType::Bool);
  auto it = pluss::codem::SeriesIterator::unary(a.nanos_, out);
  const auto* av = reinterpret_cast<const std::int64_t*>(a.nanos_.raw_data());
  auto* o = reinterpret_cast<std::uint8_t*>(out.raw_data());
  it.for_each_index([&](std::int64_t, std::int64_t ao, std::int64_t, std::int64_t oo) {
    o[oo] = (av[ao] == Duration::kNaT) ? 1 : 0;
  });
  return out;
}

pluss::codem::Series impl_isvalid_duration(const Duration& a) {
  check_duration_core_(a.nanos_);
  auto out = pluss::codem::Series::empty(a.nanos_.sizes(), pluss::codem::ScalarType::Bool);
  auto it = pluss::codem::SeriesIterator::unary(a.nanos_, out);
  const auto* av = reinterpret_cast<const std::int64_t*>(a.nanos_.raw_data());
  auto* o = reinterpret_cast<std::uint8_t*>(out.raw_data());
  it.for_each_index([&](std::int64_t, std::int64_t ao, std::int64_t, std::int64_t oo) {
    o[oo] = (av[ao] == Duration::kNaT) ? 0 : 1;
  });
  return out;
}

Duration impl_abs_duration(const Duration& a) {
  check_duration_core_(a.nanos_);
  auto out = pluss::codem::Series::empty(a.nanos_.sizes(), pluss::codem::ScalarType::Int64);
  auto it = pluss::codem::SeriesIterator::unary(a.nanos_, out);
  const auto* av = reinterpret_cast<const std::int64_t*>(a.nanos_.raw_data());
  auto* o = reinterpret_cast<std::int64_t*>(out.raw_data());
  it.for_each_index([&](std::int64_t, std::int64_t ao, std::int64_t, std::int64_t oo) {
    const auto x = av[ao];
    if (x == Duration::kNaT) o[oo] = Duration::kNaT;
    else o[oo] = (x < 0) ? -x : x;
  });
  return Duration(std::move(out));
}

Duration impl_sign_duration(const Duration& a) {
  check_duration_core_(a.nanos_);
  auto out = pluss::codem::Series::empty(a.nanos_.sizes(), pluss::codem::ScalarType::Int64);
  auto it = pluss::codem::SeriesIterator::unary(a.nanos_, out);
  const auto* av = reinterpret_cast<const std::int64_t*>(a.nanos_.raw_data());
  auto* o = reinterpret_cast<std::int64_t*>(out.raw_data());
  it.for_each_index([&](std::int64_t, std::int64_t ao, std::int64_t, std::int64_t oo) {
    const auto x = av[ao];
    if (x == Duration::kNaT) o[oo] = Duration::kNaT;
    else o[oo] = (x > 0) ? 1 : ((x < 0) ? -1 : 0);
  });
  return Duration(std::move(out));
}

Duration impl_clip_duration(const Duration& a, const Duration& lo, const Duration& hi) {
  check_duration_core_(a.nanos_);
  check_duration_core_(lo.nanos_);
  check_duration_core_(hi.nanos_);
  const auto out_sizes = broadcast_simple_(a.nanos_.sizes(), lo.nanos_.sizes());
  broadcast_simple_(out_sizes, hi.nanos_.sizes());
  auto out = pluss::codem::Series::empty(out_sizes, pluss::codem::ScalarType::Int64);
  auto it = pluss::codem::SeriesIterator::ternary(a.nanos_, lo.nanos_, hi.nanos_, out);
  const auto* av = reinterpret_cast<const std::int64_t*>(a.nanos_.raw_data());
  const auto* lv = reinterpret_cast<const std::int64_t*>(lo.nanos_.raw_data());
  const auto* hv = reinterpret_cast<const std::int64_t*>(hi.nanos_.raw_data());
  auto* o = reinterpret_cast<std::int64_t*>(out.raw_data());
  it.for_each_index_ternary([&](std::int64_t, std::int64_t ao, std::int64_t loff, std::int64_t hoff, std::int64_t oo) {
    const auto x = av[ao];
    const auto l = lv[loff];
    const auto h = hv[hoff];
    if (x == Duration::kNaT || l == Duration::kNaT || h == Duration::kNaT) { o[oo] = Duration::kNaT; return; }
    o[oo] = std::min<std::int64_t>(std::max<std::int64_t>(x, l), h);
  });
  return Duration(std::move(out));
}

Duration impl_fill_nat_duration(const Duration& a, const Duration& value) {
  check_duration_core_(a.nanos_);
  check_duration_core_(value.nanos_);
  CODEM_CHECK(value.dim() == 0, "fill_nat: value must be scalar Duration");
  const auto v = reinterpret_cast<const std::int64_t*>(value.nanos_.raw_data())[0];
  auto out = pluss::codem::Series::empty(a.nanos_.sizes(), pluss::codem::ScalarType::Int64);
  auto it = pluss::codem::SeriesIterator::unary(a.nanos_, out);
  const auto* av = reinterpret_cast<const std::int64_t*>(a.nanos_.raw_data());
  auto* o = reinterpret_cast<std::int64_t*>(out.raw_data());
  it.for_each_index([&](std::int64_t, std::int64_t ao, std::int64_t, std::int64_t oo) {
    const auto x = av[ao];
    o[oo] = (x == Duration::kNaT) ? v : x;
  });
  return Duration(std::move(out));
}

Duration impl_where_duration(const Duration& a, const pluss::codem::Series& mask, const Duration& other) {
  check_duration_core_(a.nanos_);
  check_duration_core_(other.nanos_);
  CODEM_CHECK(mask.defined(), "where: mask undefined");
  CODEM_CHECK(mask.scalar_type() == pluss::codem::ScalarType::Bool, "where: mask must be bool");
  const auto out_sizes = broadcast_simple_(a.nanos_.sizes(), other.nanos_.sizes());
  broadcast_simple_(out_sizes, mask.sizes());
  auto out = pluss::codem::Series::empty(out_sizes, pluss::codem::ScalarType::Int64);
  auto it = pluss::codem::SeriesIterator::ternary(a.nanos_, other.nanos_, mask, out);
  const auto* av = reinterpret_cast<const std::int64_t*>(a.nanos_.raw_data());
  const auto* bv = reinterpret_cast<const std::int64_t*>(other.nanos_.raw_data());
  const auto* mv = reinterpret_cast<const std::uint8_t*>(mask.raw_data());
  auto* o = reinterpret_cast<std::int64_t*>(out.raw_data());
  it.for_each_index_ternary([&](std::int64_t, std::int64_t ao, std::int64_t bo, std::int64_t mo, std::int64_t oo) {
    o[oo] = mv[mo] ? av[ao] : bv[bo];
  });
  return Duration(std::move(out));
}

Duration impl_mul_i64(const Duration& a, std::int64_t k) {
  check_duration_core_(a.nanos_);
  auto out = pluss::codem::Series::empty(a.nanos_.sizes(), pluss::codem::ScalarType::Int64);
  auto it = pluss::codem::SeriesIterator::unary(a.nanos_, out);
  const auto* av = reinterpret_cast<const std::int64_t*>(a.nanos_.raw_data());
  auto* o = reinterpret_cast<std::int64_t*>(out.raw_data());
  it.for_each_index([&](std::int64_t, std::int64_t ao, std::int64_t, std::int64_t oo) {
    const auto x = av[ao];
    o[oo] = (x == Duration::kNaT) ? Duration::kNaT : (x * k);
  });
  return Duration(std::move(out));
}

Duration impl_div_i64(const Duration& a, std::int64_t k) {
  CODEM_CHECK(k != 0, "Duration::div: division by zero");
  check_duration_core_(a.nanos_);
  auto out = pluss::codem::Series::empty(a.nanos_.sizes(), pluss::codem::ScalarType::Int64);
  auto it = pluss::codem::SeriesIterator::unary(a.nanos_, out);
  const auto* av = reinterpret_cast<const std::int64_t*>(a.nanos_.raw_data());
  auto* o = reinterpret_cast<std::int64_t*>(out.raw_data());
  it.for_each_index([&](std::int64_t, std::int64_t ao, std::int64_t, std::int64_t oo) {
    const auto x = av[ao];
    o[oo] = (x == Duration::kNaT) ? Duration::kNaT : (x / k);
  });
  return Duration(std::move(out));
}

Duration impl_floordiv_i64(const Duration& a, std::int64_t k) {
  // For int64 nanos, / already truncates toward 0; emulate floor for negatives.
  CODEM_CHECK(k != 0, "Duration::floordiv: division by zero");
  check_duration_core_(a.nanos_);
  auto out = pluss::codem::Series::empty(a.nanos_.sizes(), pluss::codem::ScalarType::Int64);
  auto it = pluss::codem::SeriesIterator::unary(a.nanos_, out);
  const auto* av = reinterpret_cast<const std::int64_t*>(a.nanos_.raw_data());
  auto* o = reinterpret_cast<std::int64_t*>(out.raw_data());
  it.for_each_index([&](std::int64_t, std::int64_t ao, std::int64_t, std::int64_t oo) {
    const auto x = av[ao];
    if (x == Duration::kNaT) { o[oo] = Duration::kNaT; return; }
    const auto q = x / k;
    const auto r = x % k;
    const bool need_floor = (r != 0) && ((r > 0) != (k > 0));
    o[oo] = need_floor ? (q - 1) : q;
  });
  return Duration(std::move(out));
}

Duration impl_mod_i64(const Duration& a, std::int64_t k) {
  CODEM_CHECK(k != 0, "Duration::mod: division by zero");
  check_duration_core_(a.nanos_);
  auto out = pluss::codem::Series::empty(a.nanos_.sizes(), pluss::codem::ScalarType::Int64);
  auto it = pluss::codem::SeriesIterator::unary(a.nanos_, out);
  const auto* av = reinterpret_cast<const std::int64_t*>(a.nanos_.raw_data());
  auto* o = reinterpret_cast<std::int64_t*>(out.raw_data());
  it.for_each_index([&](std::int64_t, std::int64_t ao, std::int64_t, std::int64_t oo) {
    const auto x = av[ao];
    o[oo] = (x == Duration::kNaT) ? Duration::kNaT : (x % k);
  });
  return Duration(std::move(out));
}

pluss::codem::Series impl_total(const Duration& a, std::string_view unit) {
  check_duration_core_(a.nanos_);
  const auto q = (double)unit_nanos_(unit);
  auto out = pluss::codem::Series::empty(a.nanos_.sizes(), pluss::codem::ScalarType::Float64);
  auto it = pluss::codem::SeriesIterator::unary(a.nanos_, out);
  const auto* av = reinterpret_cast<const std::int64_t*>(a.nanos_.raw_data());
  auto* o = reinterpret_cast<double*>(out.raw_data());
  const auto nan = std::numeric_limits<double>::quiet_NaN();
  it.for_each_index([&](std::int64_t, std::int64_t ao, std::int64_t, std::int64_t oo) {
    const auto x = av[ao];
    o[oo] = (x == Duration::kNaT) ? nan : ((double)x / q);
  });
  return out;
}

pluss::codem::Series impl_component(const Duration& a, std::string_view component) {
  check_duration_core_(a.nanos_);
  auto out = pluss::codem::Series::empty(a.nanos_.sizes(), pluss::codem::ScalarType::Int64);
  auto it = pluss::codem::SeriesIterator::unary(a.nanos_, out);
  const auto* av = reinterpret_cast<const std::int64_t*>(a.nanos_.raw_data());
  auto* o = reinterpret_cast<std::int64_t*>(out.raw_data());

  const std::int64_t day = 86400LL * 1'000'000'000LL;
  const std::int64_t hour = 3600LL * 1'000'000'000LL;
  const std::int64_t min = 60LL * 1'000'000'000LL;
  const std::int64_t sec = 1'000'000'000LL;
  const std::int64_t ms = 1'000'000LL;
  const std::int64_t us = 1'000LL;

  it.for_each_index([&](std::int64_t, std::int64_t ao, std::int64_t, std::int64_t oo) {
    const auto x = av[ao];
    if (x == Duration::kNaT) { o[oo] = Duration::kNaT; return; }
    const auto ax = (x < 0) ? -x : x;
    if (component == "days") o[oo] = x / day;
    else if (component == "hours") o[oo] = (ax % day) / hour;
    else if (component == "minutes") o[oo] = (ax % hour) / min;
    else if (component == "seconds") o[oo] = (ax % min) / sec;
    else if (component == "milliseconds") o[oo] = (ax % sec) / ms;
    else if (component == "microseconds") o[oo] = (ax % ms) / us;
    else if (component == "nanoseconds") o[oo] = (ax % us);
    else CODEM_FAIL("Duration: unknown component");
  });
  return out;
}

Duration impl_round(const Duration& a, std::string_view mode, std::string_view unit) {
  check_duration_core_(a.nanos_);
  const auto q = unit_nanos_(unit);
  auto out = pluss::codem::Series::empty(a.nanos_.sizes(), pluss::codem::ScalarType::Int64);
  auto it = pluss::codem::SeriesIterator::unary(a.nanos_, out);
  const auto* av = reinterpret_cast<const std::int64_t*>(a.nanos_.raw_data());
  auto* o = reinterpret_cast<std::int64_t*>(out.raw_data());

  it.for_each_index([&](std::int64_t, std::int64_t ao, std::int64_t, std::int64_t oo) {
    const auto x = av[ao];
    if (x == Duration::kNaT) { o[oo] = Duration::kNaT; return; }
    const auto div = x / q;
    const auto rem = x % q;
    if (mode == "floor") {
      const bool need = (rem != 0) && ((rem > 0) != (q > 0));
      o[oo] = (need ? (div - 1) : div) * q;
    } else if (mode == "ceil") {
      const bool need = (rem != 0) && ((rem > 0) == (q > 0));
      o[oo] = (need ? (div + 1) : div) * q;
    } else if (mode == "round") {
      // Round half away from zero
      const auto half = q / 2;
      const auto arem = (rem < 0) ? -rem : rem;
      const bool up = arem >= half;
      o[oo] = (div + (up ? ((x >= 0) ? 1 : -1) : 0)) * q;
    } else {
      CODEM_FAIL("Duration: unknown rounding mode");
    }
  });
  return Duration(std::move(out));
}

static Duration reduce_minmax_(const Duration& a, bool want_min) {
  CODEM_CHECK(a.defined(), "Duration reduction on undefined array");
  const auto av = impl_to_nanos_flat(a);
  CODEM_CHECK(!av.empty(), "Duration reduction on empty array");
  std::int64_t best = want_min ? (std::numeric_limits<std::int64_t>::max)()
                               : (std::numeric_limits<std::int64_t>::min)();
  bool any = false;
  for (std::size_t i = 0; i < av.size(); ++i) {
    const auto x = av[i];
    if (x == Duration::kNaT) continue;
    any = true;
    best = want_min ? std::min(best, x) : std::max(best, x);
  }
  if (!any) best = Duration::kNaT;
  return Duration::from_scalar_nanos(best);
}

Duration impl_min_duration(const Duration& a) { return reduce_minmax_(a, true); }
Duration impl_max_duration(const Duration& a) { return reduce_minmax_(a, false); }

Duration impl_sort_duration(const Duration& a) {
  check_duration_core_(a.nanos_);
  CODEM_CHECK(a.dim() == 1, "Duration::sort currently requires 1D");
  std::vector<std::int64_t> v(a.numel());
  std::memcpy(v.data(), a.nanos_.raw_data(), v.size() * sizeof(std::int64_t));
  std::stable_sort(v.begin(), v.end(), [](std::int64_t x, std::int64_t y) {
    if (x == Duration::kNaT && y == Duration::kNaT) return false;
    if (x == Duration::kNaT) return false;
    if (y == Duration::kNaT) return true;
    return x < y;
  });
  auto out = pluss::codem::Series::empty({(std::int64_t)v.size()}, pluss::codem::ScalarType::Int64);
  std::memcpy(out.raw_data(), v.data(), v.size() * sizeof(std::int64_t));
  return Duration(std::move(out));
}

Duration impl_unique_duration(const Duration& a) {
  // 1D unique preserving first occurrence, NaT treated as a distinct value.
  check_duration_core_(a.nanos_);
  CODEM_CHECK(a.dim() == 1, "Duration::unique currently requires 1D");
  std::vector<std::int64_t> outv;
  outv.reserve((size_t)a.numel());
  std::vector<std::int64_t> seen;
  seen.reserve((size_t)a.numel());
  const auto* av = reinterpret_cast<const std::int64_t*>(a.nanos_.raw_data());
  for (std::int64_t i = 0; i < a.numel(); ++i) {
    const auto x = av[i];
    if (std::find(seen.begin(), seen.end(), x) != seen.end()) continue;
    seen.push_back(x);
    outv.push_back(x);
  }
  auto out = pluss::codem::Series::empty({(std::int64_t)outv.size()}, pluss::codem::ScalarType::Int64);
  std::memcpy(out.raw_data(), outv.data(), outv.size() * sizeof(std::int64_t));
  return Duration(std::move(out));
}

pluss::codem::Series impl_argsort_duration(const Duration& a) {
  check_duration_core_(a.nanos_);
  CODEM_CHECK(a.dim() == 1, "Duration::argsort currently requires 1D");
  const auto n = a.numel();
  std::vector<std::int64_t> idx(n);
  for (std::int64_t i = 0; i < n; ++i) idx[i] = i;
  const auto* av = reinterpret_cast<const std::int64_t*>(a.nanos_.raw_data());
  std::stable_sort(idx.begin(), idx.end(), [&](std::int64_t i, std::int64_t j) {
    const auto x = av[i], y = av[j];
    if (x == Duration::kNaT && y == Duration::kNaT) return i < j;
    if (x == Duration::kNaT) return false;
    if (y == Duration::kNaT) return true;
    if (x == y) return i < j;
    return x < y;
  });
  auto out = pluss::codem::Series::empty({n}, pluss::codem::ScalarType::Int64);
  std::memcpy(out.raw_data(), idx.data(), idx.size() * sizeof(std::int64_t));
  return out;
}

std::vector<std::int64_t> impl_to_nanos_flat(const Duration& a) {
  check_duration_core_(a.nanos_);
  std::vector<std::int64_t> v((size_t)a.numel());
  std::memcpy(v.data(), a.nanos_.raw_data(), v.size() * sizeof(std::int64_t));
  return v;
}

static std::string format_one_(std::int64_t ns, bool compact) {
  if (ns == Duration::kNaT) return "NaT";
  bool neg = ns < 0;
  std::int64_t a = neg ? -ns : ns;
  const std::int64_t day = 86400LL * 1'000'000'000LL;
  const std::int64_t hour = 3600LL * 1'000'000'000LL;
  const std::int64_t min = 60LL * 1'000'000'000LL;
  const std::int64_t sec = 1'000'000'000LL;
  const auto d = a / day; a %= day;
  const auto h = a / hour; a %= hour;
  const auto m = a / min; a %= min;
  const auto s = a / sec; a %= sec;
  const auto n = a;
  char buf[128];
  if (compact) {
    std::snprintf(buf, sizeof(buf), "%s%lld %02lld:%02lld:%02lld", neg?"-":"", (long long)d, (long long)h, (long long)m, (long long)s);
  } else {
    std::snprintf(buf, sizeof(buf), "%s%lld days %02lld:%02lld:%02lld.%09lld", neg?"-":"", (long long)d, (long long)h, (long long)m, (long long)s, (long long)n);
  }
  return std::string(buf);
}

pluss::coden::Note impl_duration_to_note(const Duration& a, std::string_view style) {
  check_duration_core_(a.nanos_);
  // Build flattened string vector then use Note factory.
  std::vector<std::string> ss;
  ss.reserve((size_t)a.numel());
  const auto* av = reinterpret_cast<const std::int64_t*>(a.nanos_.raw_data());
  const bool compact = (style == "compact");
  for (std::int64_t i = 0; i < a.numel(); ++i) ss.push_back(format_one_(av[i], compact));
  std::vector<std::string_view> sv; sv.reserve(ss.size());
  for (auto& s : ss) sv.push_back(s);
  return ::pluss::coden::ops_internal::note_from_utf8(sv, a.sizes());
}

} // namespace detail

} // namespace pluss::codeh
