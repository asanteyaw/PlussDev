#pragma once
#include "CodeM/core/Series.h"

namespace pluss::codem::native::cpu {
  Series scatter_add(const Series& input, std::int64_t dim, const Series& index, const Series& src);
  void scatter_add_inplace(Series& input, std::int64_t dim, const Series& index, const Series& src);
}
