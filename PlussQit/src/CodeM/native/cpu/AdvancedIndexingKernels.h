#pragma once
#include "CodeM/core/Series.h"
#include <vector>

namespace pluss::codem::native::cpu {
  Series index_put(const Series& a, const std::vector<Series>& indices, const Series& values, bool accumulate);
  void index_put_inplace(Series& a, const std::vector<Series>& indices, const Series& values, bool accumulate);
} // namespace pluss::codem::native::cpu
