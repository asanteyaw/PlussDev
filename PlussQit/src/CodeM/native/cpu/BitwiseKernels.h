#pragma once
#include "CodeM/core/Series.h"

namespace pluss::codem::native::cpu {
  void bitwise_and(const Series& a, const Series& b, const Series& out);
  void bitwise_or(const Series& a, const Series& b, const Series& out);
  void bitwise_xor(const Series& a, const Series& b, const Series& out);
  void bitwise_not(const Series& a, Series& out);
}
