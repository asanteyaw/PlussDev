#pragma once
#include "CodeM/core/SeriesSlim.h"

namespace pluss::codem::native::cpu {
  void cumsum_dim(const Series& a, std::int64_t dim, Series& out);
  void cumprod_dim(const Series& a, std::int64_t dim, Series& out);
}
