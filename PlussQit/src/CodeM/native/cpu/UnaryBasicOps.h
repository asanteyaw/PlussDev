#pragma once
#include "CodeM/core/Series.h"

namespace pluss::codem::native::cpu {
  void sqrt(const Series& a, Series& out);
  void floor(const Series& a, Series& out);
  void ceil(const Series& a, Series& out);
  void round(const Series& a, Series& out);
  void trunc(const Series& a, Series& out);
  void isnan(const Series& a, Series& out);
  void isinf(const Series& a, Series& out);
}
