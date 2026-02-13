#pragma once
#include "CodeM/core/Series.h"

namespace pluss::codem::native::cpu {
  void sum_dim(const Series& a, std::int64_t dim, Series& out);
  void prod_dim(const Series& a, std::int64_t dim, Series& out);
  void amax_dim(const Series& a, std::int64_t dim, Series& out);
  void amin_dim(const Series& a, std::int64_t dim, Series& out);
  void all_dim(const Series& a, std::int64_t dim, Series& out);
  void any_dim(const Series& a, std::int64_t dim, Series& out);

  void sum_all(const Series& a, Series& out);
  void mean_all(const Series& a, Series& out);
}
