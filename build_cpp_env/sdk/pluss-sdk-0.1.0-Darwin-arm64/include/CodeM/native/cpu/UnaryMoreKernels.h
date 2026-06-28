#pragma once
#include "CodeM/core/SeriesSlim.h"

namespace pluss::codem::native::cpu {
  // Many unary floating-point ops (Float16/BFloat16/Float32/Float64).
  void tan(const Series& a, Series& out);
  void asin(const Series& a, Series& out);
  void acos(const Series& a, Series& out);
  void atan(const Series& a, Series& out);
  void sinh(const Series& a, Series& out);
  void cosh(const Series& a, Series& out);

  void log1p(const Series& a, Series& out);
  void expm1(const Series& a, Series& out);
  void erf(const Series& a, Series& out);
  void erfc(const Series& a, Series& out);
  void lgamma(const Series& a, Series& out);
  void asinh(const Series& a, Series& out);
  void acosh(const Series& a, Series& out);
  void atanh(const Series& a, Series& out);
  void isnan(const Series& a, Series& out);     // bool out
  void isinf(const Series& a, Series& out);     // bool out
  void isfinite(const Series& a, Series& out);  // bool out

  void floor(const Series& a, Series& out);
  void ceil(const Series& a, Series& out);
  void round(const Series& a, Series& out);
  void trunc(const Series& a, Series& out);

  void reciprocal(const Series& a, Series& out);
  void sigmoid(const Series& a, Series& out);
  void relu(const Series& a, Series& out);

  void logical_not(const Series& a, Series& out); // bool -> bool
}
