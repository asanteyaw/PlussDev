#pragma once
#include "CodeM/core/SeriesSlim.h"

namespace pluss::codem::native::cpu {
  Series mean(const Series& a, std::int64_t dim, bool keepdim);
  Series amax(const Series& a, std::int64_t dim, bool keepdim);
  Series amin(const Series& a, std::int64_t dim, bool keepdim);
  Series argmax(const Series& a, std::int64_t dim, bool keepdim);
  Series argmin(const Series& a, std::int64_t dim, bool keepdim);
}
