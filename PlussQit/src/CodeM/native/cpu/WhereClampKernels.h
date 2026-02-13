#pragma once
#include "CodeM/core/Series.h"

namespace pluss::codem::native::cpu {

  // where(cond, a, b): cond bool, returns same dtype as promote(a,b), currently requires same shape.
  void where(const Series& cond, const Series& a, const Series& b, Series& out);

  // clamp with scalar bounds (same dtype)
  void clamp_inplace(const Series& a, double min_v, double max_v);
  void relu_inplace(const Series& a);
  void clamp_min(const Series& a, double min_v, Series& out);
  void clamp_max(const Series& a, double max_v, Series& out);

  // nan_to_num: for float dtypes
  void nan_to_num(const Series& a, double nan, double posinf, double neginf, Series& out);

} // namespace
