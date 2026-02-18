#include "CodeH/core/Duration.h"

#include <limits>
#include <cstring>

namespace pluss::codeh {

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

Duration Duration::from_scalar_nanos(std::int64_t nanos) {
  auto s = pluss::codem::Series::empty({}, pluss::codem::ScalarType::Int64);
  *reinterpret_cast<std::int64_t*>(s.raw_data()) = nanos;
  return Duration(std::move(s));
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
    o[oo] = (av == kNaT || bv == kNaT) ? kNaT : (av + bv);
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
    o[oo] = (av == kNaT || bv == kNaT) ? kNaT : (av - bv);
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

  std::vector<std::int64_t> flat;
  if (inc > 0) {
    for (std::int64_t v = s; v <= e; v += inc) flat.push_back(v);
  } else {
    for (std::int64_t v = s; v >= e; v += inc) flat.push_back(v);
  }

  auto out = pluss::codem::Series::empty({(std::int64_t)flat.size()}, pluss::codem::ScalarType::Int64);
  std::memcpy(out.raw_data(), flat.data(), flat.size() * sizeof(std::int64_t));
  return Duration(std::move(out));
}

namespace ops_internal {

Duration duration_from_nanos(std::span<const std::int64_t> nanos_flat,
                             std::span<const std::int64_t> sizes) {
  std::vector<std::int64_t> shape(sizes.begin(), sizes.end());
  if (shape.empty()) {
    CODEM_CHECK(nanos_flat.size() == 1, "duration_from_nanos: scalar expects 1 element");
  } else {
    std::int64_t prod = 1;
    for (const auto s : shape) prod *= s;
    CODEM_CHECK(prod == static_cast<std::int64_t>(nanos_flat.size()),
                "duration_from_nanos: flat size mismatch");
  }

  auto out = pluss::codem::Series::empty(shape, pluss::codem::ScalarType::Int64);
  if (!nanos_flat.empty()) {
    std::memcpy(out.raw_data(), nanos_flat.data(),
                nanos_flat.size() * sizeof(std::int64_t));
  }
  return Duration(std::move(out));
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
