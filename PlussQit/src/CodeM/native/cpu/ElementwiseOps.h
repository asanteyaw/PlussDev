#pragma once
#include "CodeM/core/Series.h"

namespace pluss::codem::native::cpu {
  void add(const Series& a, const Series& b, const Series& out);
  void sub(const Series& a, const Series& b, const Series& out);
  void mul(const Series& a, const Series& b, const Series& out);
  void div(const Series& a, const Series& b, const Series& out);

  void neg(const Series& a, Series& out);
  void abs(const Series& a, Series& out);
}
