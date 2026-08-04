#pragma once
#include "CodeM/core/SeriesSlim.h"
#include <cstdint>

namespace pluss::codem::native::cpu {
  void add(const Series& a, const Series& b, const Series& out);
  void sub(const Series& a, const Series& b, const Series& out);
  void mul(const Series& a, const Series& b, const Series& out);
  void div(const Series& a, const Series& b, const Series& out);

  // Integer-exact scalar variants: a is any integral dtype, s is a native
  // std::int64_t (no double round-trip). div_i64/mod_i64 truncate toward zero;
  // caller is responsible for s != 0. See Arithmetic.cpp for the fast path.
  void add_i64(const Series& a, std::int64_t s, const Series& out);
  void sub_i64(const Series& a, std::int64_t s, const Series& out);
  void mul_i64(const Series& a, std::int64_t s, const Series& out);
  void div_i64(const Series& a, std::int64_t s, const Series& out);
  void mod_i64(const Series& a, std::int64_t s, const Series& out);

  void clamp(const Series& a, Series& out, double lo, double hi);
} // namespace pluss::codem::native::cpu
