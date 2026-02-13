#pragma once
#include "CodeM/core/Series.h"

namespace pluss::codem::native::cpu {
  Series gather(const Series& input, std::int64_t dim, const Series& index);
  void scatter_(Series& input, std::int64_t dim, const Series& index, const Series& src);
  Series scatter(const Series& input, std::int64_t dim, const Series& index, const Series& src);
}
