#pragma once
#include "CodeM/core/SeriesSlim.h"

namespace pluss::codem::native::cpu {
  Series where_scalar(const Series& cond, const Series& a, const Series& b); // reserved
  Series masked_fill(const Series& a, const Series& mask, double value);
  void masked_fill_inplace(Series& a, const Series& mask, double value);
}
