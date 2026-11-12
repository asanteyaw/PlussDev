#include "CodeH/core/Duration.h"

#include "pluss/codeh/internal/timeline_factory_detail.h"
#include "CodeH/core/detail/WideDivide.h"

#include <limits>
#include <cstring>

namespace pluss::codeh {

namespace {

std::int64_t checked_duration_nanos(__int128 value, const char* operation) {
  CODEM_CHECK(value >= (std::numeric_limits<std::int64_t>::min)() + 1 &&
                  value <= (std::numeric_limits<std::int64_t>::max)(),
              std::string("Duration::") + operation + ": nanosecond overflow");
  return static_cast<std::int64_t>(value);
}

} // namespace

static void check_duration_core(const pluss::codem::Series& s) {
  CODEM_CHECK(s.defined(), "Duration: underlying Series is undefined");
  CODEM_CHECK(s.scalar_type() == pluss::codem::ScalarType::Int64, "Duration: nanos must be int64");
  CODEM_CHECK(s.dim() >= 0 && s.dim() <= 3, "Duration: only supports 0D..3D");
}

static std::vector<std::int64_t> broadcast_simple(std::span<const std::int64_t> a,
                                                 std::span<const std::int64_t> b) {
  if (a.size() == 0) return std::vector<std::int64_t>(b.begin(), b.end());
  if (b.size() == 0) return std::vector<std::int64_t>(a.begin(), a.end());
  CODEM_CHECK(a.size() == b.size(), "Duration: shape mismatch (dims differ)");
  for (size_t i = 0; i < a.size(); ++i) {
    CODEM_CHECK(a[i] == b[i], "Duration: shape mismatch");
  }
  return std::vector<std::int64_t>(a.begin(), a.end());
}

Duration Duration::operator+(const Duration& other) const {
  check_duration_core(nanos_);
  check_duration_core(other.nanos_);

  const auto out_sizes = broadcast_simple(nanos_.sizes(), other.nanos_.sizes());
  auto out = pluss::codem::Series::empty(out_sizes, pluss::codem::ScalarType::Int64);

  auto it = pluss::codem::SeriesIterator::binary(nanos_, other.nanos_, out);
  const auto* a = reinterpret_cast<const std::int64_t*>(nanos_.raw_data());
  const auto* b = reinterpret_cast<const std::int64_t*>(other.nanos_.raw_data());
  auto* o = reinterpret_cast<std::int64_t*>(out.raw_data());

  it.for_each_index([&](std::int64_t /*lin*/, std::int64_t ao, std::int64_t bo, std::int64_t oo) {
    const auto av = a[ao];
    const auto bv = b[bo];
    o[oo] = (av == kNaT || bv == kNaT)
        ? kNaT
        : checked_duration_nanos(static_cast<__int128>(av) + bv, "operator+");
  });

  return Duration(std::move(out));
}

Duration Duration::operator-(const Duration& other) const {
  check_duration_core(nanos_);
  check_duration_core(other.nanos_);

  const auto out_sizes = broadcast_simple(nanos_.sizes(), other.nanos_.sizes());
  auto out = pluss::codem::Series::empty(out_sizes, pluss::codem::ScalarType::Int64);

  auto it = pluss::codem::SeriesIterator::binary(nanos_, other.nanos_, out);
  const auto* a = reinterpret_cast<const std::int64_t*>(nanos_.raw_data());
  const auto* b = reinterpret_cast<const std::int64_t*>(other.nanos_.raw_data());
  auto* o = reinterpret_cast<std::int64_t*>(out.raw_data());

  it.for_each_index([&](std::int64_t /*lin*/, std::int64_t ao, std::int64_t bo, std::int64_t oo) {
    const auto av = a[ao];
    const auto bv = b[bo];
    o[oo] = (av == kNaT || bv == kNaT)
        ? kNaT
        : checked_duration_nanos(static_cast<__int128>(av) - bv, "operator-");
  });

  return Duration(std::move(out));
}

Duration Duration::operator-() const {
  check_duration_core(nanos_);
  auto out = pluss::codem::Series::empty(nanos_.sizes(), pluss::codem::ScalarType::Int64);

  auto it = pluss::codem::SeriesIterator::unary(nanos_, out);
  const auto* a = reinterpret_cast<const std::int64_t*>(nanos_.raw_data());
  auto* o = reinterpret_cast<std::int64_t*>(out.raw_data());

  it.for_each_index([&](std::int64_t /*lin*/, std::int64_t ao, std::int64_t /*bo*/, std::int64_t oo) {
    const auto av = a[ao];
    o[oo] = (av == kNaT) ? kNaT : (-av);
  });

  return Duration(std::move(out));
}

Duration Duration::range(const Duration& start, const Duration& end, const Duration& step) {
  check_duration_core(start.nanos_);
  check_duration_core(end.nanos_);
  check_duration_core(step.nanos_);
  CODEM_CHECK(start.dim() == 0 && end.dim() == 0 && step.dim() == 0, "Duration::range requires scalar args");
  const auto s = reinterpret_cast<const std::int64_t*>(start.nanos_.raw_data())[0];
  const auto e = reinterpret_cast<const std::int64_t*>(end.nanos_.raw_data())[0];
  const auto inc = reinterpret_cast<const std::int64_t*>(step.nanos_.raw_data())[0];
  CODEM_CHECK(s != kNaT && e != kNaT && inc != kNaT, "Duration::range does not allow NaT args");
  CODEM_CHECK(inc != 0, "Duration::range step must be non-zero");

  const bool advances = inc > 0 ? s <= e : s >= e;
  __int128 count_wide = 0;
  if (advances) {
    const auto distance = inc > 0 ? static_cast<__int128>(e) - s
                                  : static_cast<__int128>(s) - e;
    const auto stride = inc > 0 ? static_cast<__int128>(inc)
                                : -static_cast<__int128>(inc);
    count_wide = detail::divide_bounded_wide_by_u64(
                     distance, static_cast<std::uint64_t>(stride)) + 1;
  }
  CODEM_CHECK(count_wide <= (std::numeric_limits<std::int64_t>::max)(),
              "Duration::range element count overflows int64");
  const auto count = static_cast<std::int64_t>(count_wide);
  std::vector<std::int64_t> flat;
  flat.reserve(static_cast<std::size_t>(count));
  for (std::int64_t i = 0; i < count; ++i) {
    flat.push_back(static_cast<std::int64_t>(
        static_cast<__int128>(s) + static_cast<__int128>(i) * inc));
  }

  auto out = pluss::codem::Series::empty({(std::int64_t)flat.size()}, pluss::codem::ScalarType::Int64);
  std::memcpy(out.raw_data(), flat.data(), flat.size() * sizeof(std::int64_t));
  return Duration(std::move(out));
}

namespace ops_internal {

Duration duration_from_nanos(std::span<const std::int64_t> nanos_flat,
                             std::span<const std::int64_t> sizes) {
  std::vector<std::int64_t> shape(sizes.begin(), sizes.end());
  const auto expected = pluss::codeh::factory_detail::checked_numel(
      pluss::msharp::RangeView::Int(shape), "Duration construction");
  CODEM_CHECK(expected == static_cast<std::int64_t>(nanos_flat.size()),
              shape.empty() ? "duration_from_nanos: scalar expects 1 element"
                            : "duration_from_nanos: flat size mismatch");

  auto out = pluss::codem::Series::empty(shape, pluss::codem::ScalarType::Int64);
  if (!nanos_flat.empty()) {
    std::memcpy(out.raw_data(), nanos_flat.data(),
                nanos_flat.size() * sizeof(std::int64_t));
  }
  return Duration(std::move(out));
}

Duration duration_from_series(pluss::codem::Series nanos) {
  CODEM_CHECK(nanos.scalar_type() == pluss::codem::ScalarType::Int64,
              "duration_from_series: expected an Int64 nanoseconds Series");
  return Duration(std::move(nanos));
}

} // namespace ops_internal

namespace detail {

Duration impl_add_duration(const Duration& a, const Duration& b) { return a + b; }
Duration impl_sub_duration(const Duration& a, const Duration& b) { return a - b; }
Duration impl_neg_duration(const Duration& a) { return -a; }
Duration impl_range(const Duration& start, const Duration& end, const Duration& step) {
  return Duration::range(start, end, step);
}

} // namespace detail

} // namespace pluss::codeh
