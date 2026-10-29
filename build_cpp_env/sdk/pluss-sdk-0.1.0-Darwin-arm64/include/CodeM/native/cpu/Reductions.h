#pragma once
#include "CodeM/core/SeriesSlim.h"
#include <utility>

namespace pluss::codem::native::cpu {
  Series mean(const Series& a, std::int64_t dim, bool keepdim);
  Series amax(const Series& a, std::int64_t dim, bool keepdim);
  Series amin(const Series& a, std::int64_t dim, bool keepdim);
  Series argmax(const Series& a, std::int64_t dim, bool keepdim);
  Series argmin(const Series& a, std::int64_t dim, bool keepdim);
  std::pair<Series, Series> max_pair(const Series& a, std::int64_t dim, bool keepdim);
  std::pair<Series, Series> min_pair(const Series& a, std::int64_t dim, bool keepdim);
  Series sum_all(const Series& a);
  Series all_all(const Series& a);
  Series any_all(const Series& a);
}
