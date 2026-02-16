#pragma once
#include "CodeM/core/SeriesSlim.h"

namespace pluss::codem::native::cpu {
  void add(const Series& a, const Series& b, const Series& out);
  void sub(const Series& a, const Series& b, const Series& out);
  void mul(const Series& a, const Series& b, const Series& out);
  void div(const Series& a, const Series& b, const Series& out);

  void clamp(const Series& a, Series& out, double lo, double hi);
} // namespace pluss::codem::native::cpu
