#pragma once
#include "CodeM/core/Series.h"

namespace pluss::codem::ops_internal {
  Series add(const Series& a, const Series& b);
  Series sub(const Series& a, const Series& b);
  Series mul(const Series& a, const Series& b);
  Series div(const Series& a, const Series& b);
  Series pow(const Series& a, const Series& b);

  Series abs(const Series& a);
  Series neg(const Series& a);

  Series eq(const Series& a, const Series& b);
  Series ne(const Series& a, const Series& b);
  Series lt(const Series& a, const Series& b);
  Series le(const Series& a, const Series& b);
  Series gt(const Series& a, const Series& b);
  Series ge(const Series& a, const Series& b);

  Series sqrt(const Series& a);
  Series floor(const Series& a);
  Series ceil(const Series& a);
  Series round(const Series& a);
  Series trunc(const Series& a);
  Series isnan(const Series& a);
  Series isinf(const Series& a);
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


// Reductions (CPU-only, Torch-inspired)
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

  Series argmax(const Series& a, std::int64_t dim=-1, bool keepdim=false);
  Series argmin(const Series& a, std::int64_t dim=-1, bool keepdim=false);

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

  // Scatter (Torch-inspired CPU-only)
  Series scatter_add(const Series& a, std::int64_t dim, const Series& index, const Series& src);
  Series& scatter_add_(Series& a, std::int64_t dim, const Series& index, const Series& src);
  Series scatter_reduce(const Series& a, std::int64_t dim, const Series& index, const Series& src, const std::string& reduce, bool include_self=true);
  Series& scatter_reduce_(Series& a, std::int64_t dim, const Series& index, const Series& src, const std::string& reduce, bool include_self=true);

  // Indexing helpers
  Series gather(const Series& input, std::int64_t dim, const Series& index);

Series maximum(const Series& a, const Series& b);
Series minimum(const Series& a, const Series& b);
Series fmod(const Series& a, const Series& b);
Series remainder(const Series& a, const Series& b);
Series atan2(const Series& a, const Series& b);
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
  Series bincount(const Series& a, std::int64_t minlength=0);
  Series histc(const Series& a, std::int64_t bins=100, double min=0.0, double max=0.0);
  Series masked_scatter(const Series& a, const Series& mask, const Series& src);
  Series& masked_scatter_(Series& a, const Series& mask, const Series& src);


  // Where variants (method-form: condition.where(x,y))
  Series where(const Series& condition, const Series& x, const Series& y);

  // Nonzero (returns indices, int64)
  Series nonzero(const Series& a);

} // namespace pluss::codem::ops_internal
