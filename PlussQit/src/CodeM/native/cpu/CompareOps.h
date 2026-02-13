#pragma once
#include "CodeM/core/Series.h"

namespace pluss::codem::native::cpu {
  void eq(const Series& a, const Series& b, const Series& out);
  void ne(const Series& a, const Series& b, const Series& out);
  void lt(const Series& a, const Series& b, const Series& out);
  void le(const Series& a, const Series& b, const Series& out);
  void gt(const Series& a, const Series& b, const Series& out);
  void ge(const Series& a, const Series& b, const Series& out);
}
