#pragma once
#include "CodeM/core/Series.h"
#include <cstdint>
#include <string_view>
#include <vector>

namespace pluss::codem::ops_internal {

// Internal construction hook: ops_internal implementations can materialize a
// Series from an implementation pointer, while end users cannot.
struct SeriesFactoryAccess {
  static Series wrap(pluss::msharp::intrusive_ptr<SeriesImpl> impl) {
    return Series(std::move(impl));
  }
};

// Scoped default platform for internal temporaries created via Series::empty(..., dtype).
// Used by dispatch code to keep operation outputs on the active tensor platform.
class PlatformScope final {
 public:
  explicit PlatformScope(Platform p) noexcept;
  ~PlatformScope();
  PlatformScope(const PlatformScope&) = delete;
  PlatformScope& operator=(const PlatformScope&) = delete;

 private:
  bool active_{false};
};

// Enforce same-platform semantics for tensor interop ops.
void require_same_platform(const Series& a, const Series& b, std::string_view op);

  Series add(const Series& a, const Series& b);
  Series sub(const Series& a, const Series& b);
  Series mul(const Series& a, const Series& b);
    Series addcmul(const Series& a, const Series& b, const Series& c, double value=1.0);
    Series& addcmul_(Series& a, const Series& b, const Series& c, double value=1.0);
    Series addcdiv(const Series& a, const Series& b, const Series& c, double value=1.0);
    Series& addcdiv_(Series& a, const Series& b, const Series& c, double value=1.0);
  Series div(const Series& a, const Series& b);
  Series pow(const Series& a, const Series& b);
  // Scalar variants
  Series add(const Series& a, double s);
  Series sub(const Series& a, double s);
  Series mul(const Series& a, double s);
  Series div(const Series& a, double s);
  Series pow(const Series& a, double s);

  // Integer-exact scalar variants: a must be an integral dtype; s stays a
  // native std::int64_t with no double round-trip, correct for the full
  // int64_t range (the double-based scalar ops above only round-trip
  // exactly for |s| < 2^53). div_i64/mod_i64 truncate toward zero and
  // PLUSS_CHECK(s != 0, ...) -- distinct from div(a,b)/div(a,double) above,
  // which always promote integral output to Float64 ("true division").
  Series add_i64(const Series& a, std::int64_t s);
  Series sub_i64(const Series& a, std::int64_t s);
  Series mul_i64(const Series& a, std::int64_t s);
  Series div_i64(const Series& a, std::int64_t s);
  Series mod_i64(const Series& a, std::int64_t s);

  Series abs(const Series& a);
  Series& abs_(Series& a);
  Series neg(const Series& a);

  Series eq(const Series& a, const Series& b);
  Series ne(const Series& a, const Series& b);
  Series lt(const Series& a, const Series& b);
  Series le(const Series& a, const Series& b);
  Series gt(const Series& a, const Series& b);
  Series ge(const Series& a, const Series& b);

  Series sqrt(const Series& a);
  // Extra unary math used by the compat invoke layer.
  Series exp(const Series& a);
  Series log(const Series& a);

  // In-place variants used by the compat invoke layer.
  Series& exp_(Series& a);
  Series& log_(Series& a);
  Series& sqrt_(Series& a);
  Series floor(const Series& a);
  Series ceil(const Series& a);
  Series round(const Series& a);
  Series trunc(const Series& a);
  Series isnan(const Series& a);
  Series isinf(const Series& a);
  Series signbit(const Series& a);
  Series invalid(const Series& a);
  Series invalid_index(const Series& a);
  // In-place arithmetic (scalar and elementwise)
  Series& add_(Series& a, double s);
  Series& sub_(Series& a, double s);
  Series& mul_(Series& a, double s);
  Series& div_(Series& a, double s);
  Series& add_(Series& a, const Series& b);
  Series& sub_(Series& a, const Series& b);
  Series& mul_(Series& a, const Series& b);
  Series& div_(Series& a, const Series& b);
  // Unary math (CPU-only)
  Series sin(const Series& a);
  Series cos(const Series& a);
  Series tan(const Series& a);
  Series asin(const Series& a);
  Series acos(const Series& a);
  Series atan(const Series& a);
  Series sinh(const Series& a);
  Series cosh(const Series& a);
  Series tanh(const Series& a);

  Series log1p(const Series& a);
  Series expm1(const Series& a);
  Series reciprocal(const Series& a);

  Series erf(const Series& a);
  Series erfc(const Series& a);
  Series lgamma(const Series& a);
  Series digamma(const Series& a);
  Series asinh(const Series& a);
  Series acosh(const Series& a);
  Series atanh(const Series& a);

  Series isfinite(const Series& a);


  // Logical (bool out)
  Series logical_not(const Series& a);
  Series logical_and(const Series& a, const Series& b);
  Series logical_or(const Series& a, const Series& b);
  Series logical_xor(const Series& a, const Series& b);

  // Masks
  Series masked_select(const Series& a, const Series& mask);
  Series masked_fill(const Series& a, const Series& mask, const Series& value);
  Series masked_fill(const Series& a, const Series& mask, double value);
  Series& masked_fill_(Series& a, const Series& mask, const Series& value);
  Series& masked_fill_(Series& a, const Series& mask, double value);


// Reductions (CPU-only)
  Series sum(const Series& a);
  Series sum(const Series& a, std::int64_t dim, bool keepdim=false);
  Series mean(const Series& a);
  Series mean(const Series& a, std::int64_t dim, bool keepdim=false);
  Series prod(const Series& a);
  Series prod(const Series& a, std::int64_t dim, bool keepdim=false);
  Series amax(const Series& a);
  Series amax(const Series& a, std::int64_t dim, bool keepdim=false);
  Series amin(const Series& a);
  Series amin(const Series& a, std::int64_t dim, bool keepdim=false);

  Series all(const Series& a);
  Series all(const Series& a, std::int64_t dim, bool keepdim=false);
  Series any(const Series& a);
  Series any(const Series& a, std::int64_t dim, bool keepdim=false);

  Series argmax(const Series& a);
  Series argmax(const Series& a, std::int64_t dim, bool keepdim=false);
  Series argmin(const Series& a);
  Series argmin(const Series& a, std::int64_t dim, bool keepdim=false);

  // Complex helpers
  Series conj(const Series& a);
  Series real(const Series& a);
  Series imag(const Series& a);
  Series angle(const Series& a);

  // Ternary and masking
  Series where(const Series& cond, const Series& a, const Series& b);
  Series masked_fill(const Series& a, const Series& mask, double value);
  Series& masked_fill_(Series& a, const Series& mask, double value);

  // Clamping / activations
  Series softmax(const Series& a, std::int64_t dim);
  Series log_softmax(const Series& a, std::int64_t dim);
  Series logsumexp(const Series& a, std::int64_t dim, bool keepdim=false);

  // Clamping / activations
  Series sigmoid(const Series& a);
  Series& sigmoid_(Series& a);
  Series softplus(const Series& a, double beta=1.0, double threshold=20.0);
  Series& softplus_(Series& a, double beta=1.0, double threshold=20.0);
  Series silu(const Series& a);
  Series& silu_(Series& a);
  Series gelu(const Series& a, bool approximate=true);
  Series& gelu_(Series& a, bool approximate=true);
  Series leaky_relu(const Series& a, double negative_slope=0.01);
  Series& leaky_relu_(Series& a, double negative_slope=0.01);


  // Clamping / activations
  Series clamp(const Series& a, double min, double max);
  Series& clamp_(Series& a, double min, double max);
  Series relu(const Series& a);
  Series& relu_(Series& a);

  // Scatter (CPU-only)
  Series scatter(const Series& a, std::int64_t dim, const Series& index, const Series& src);
  Series& scatter_(Series& a, std::int64_t dim, const Series& index, const Series& src);
  Series scatter_add(const Series& a, std::int64_t dim, const Series& index, const Series& src);
  Series& scatter_add_(Series& a, std::int64_t dim, const Series& index, const Series& src);
  Series scatter_reduce(const Series& a, std::int64_t dim, const Series& index, const Series& src, const std::string& reduce, bool include_self=true);
  Series scatter_reduce(const Series& a, std::int64_t dim, const Series& index, const Series& src, const std::string& reduce, bool include_self, bool has_initial, double initial);
  Series& scatter_reduce_(Series& a, std::int64_t dim, const Series& index, const Series& src, const std::string& reduce, bool include_self=true);
  Series& scatter_reduce_(Series& a, std::int64_t dim, const Series& index, const Series& src, const std::string& reduce, bool include_self, bool has_initial, double initial);

  // Indexing helpers
  Series gather(const Series& input, std::int64_t dim, const Series& index);

  Series maximum(const Series& a, const Series& b);
  Series minimum(const Series& a, const Series& b);
  Series fmod(const Series& a, const Series& b);
  Series remainder(const Series& a, const Series& b);
  Series atan2(const Series& a, const Series& b);
  Series hypot(const Series& a, const Series& b);
  Series logaddexp(const Series& a, const Series& b);
  Series logaddexp2(const Series& a, const Series& b);
  Series xlogy(const Series& a, const Series& b);
  Series clamp_min(const Series& a, double lo);
  Series& clamp_min_(Series& a, double lo);
  Series clamp_max(const Series& a, double hi);
  Series& clamp_max_(Series& a, double hi);
  Series rsqrt(const Series& a);
  Series log2(const Series& a);
  Series log10(const Series& a);
  Series exp2(const Series& a);
  Series cbrt(const Series& a);
  Series square(const Series& a);
  Series nan_to_num(const Series& a, double nan=0.0, double posinf=0.0, double neginf=0.0);
  Series& nan_to_num_(Series& a, double nan=0.0, double posinf=0.0, double neginf=0.0);


  // Counting / histogram / unique (CPU-only)
  Series unique(const Series& a, bool sorted=true);
  Series unique(const Series& a, std::int64_t dim, bool sorted=true);
  std::vector<Series> unique(const Series& a,
                             std::int64_t dim,
                             bool sorted,
                             bool return_inverse,
                             bool return_counts);
  Series isin(const Series& elements, const Series& test_elements);
  Series isin(const Series& elements, const Series& test_elements, bool assume_unique);
  Series isin(const Series& elements,
              const Series& test_elements,
              bool assume_unique,
              bool invert);
  Series union1d(const Series& a, const Series& b);
  Series union1d(const Series& a, const Series& b, bool sorted);
  Series intersect1d(const Series& a, const Series& b);
  Series intersect1d(const Series& a, const Series& b, bool sorted);
  Series intersect1d(const Series& a,
                     const Series& b,
                     bool sorted,
                     bool assume_unique);
  Series setdiff1d(const Series& a, const Series& b);
  Series setdiff1d(const Series& a, const Series& b, bool sorted);
  Series setdiff1d(const Series& a,
                   const Series& b,
                   bool sorted,
                   bool assume_unique);
  Series setxor1d(const Series& a, const Series& b);
  Series setxor1d(const Series& a, const Series& b, bool sorted);
  Series setxor1d(const Series& a,
                  const Series& b,
                  bool sorted,
                  bool assume_unique);
    // Shape ops
  Series squeeze(const Series& a);
  Series squeeze(const Series& a, std::int64_t dim);
  Series& squeeze_(Series& a);
  Series& squeeze_(Series& a, std::int64_t dim);

  // Bitwise ops (CPU-only)
  Series bitwise_not(const Series& a);
  Series& bitwise_not_(Series& a);

  Series bitwise_and(const Series& a, const Series& b);
  Series& bitwise_and_(Series& a, const Series& b);

  Series bitwise_or(const Series& a, const Series& b);
  Series& bitwise_or_(Series& a, const Series& b);

  Series bitwise_xor(const Series& a, const Series& b);
  Series& bitwise_xor_(Series& a, const Series& b);
  
  Series bincount(const Series& a, std::int64_t minlength=0);
  Series histc(const Series& a, std::int64_t bins=100, double min=0.0, double max=0.0);
  Series masked_scatter(const Series& a, const Series& mask, const Series& src);
  Series& masked_scatter_(Series& a, const Series& mask, const Series& src);


  // Where variants (method-form: condition.where(x,y))
  Series where(const Series& condition, const Series& x, const Series& y);

  // Nonzero (returns indices, int64)
  Series nonzero(const Series& a);

} // namespace pluss::codem::ops_internal
