#pragma once
#include <cstdint>
#include <utility>
#include "CodeM/core/SeriesSlim.h"

namespace pluss::codem::native::cpu {

// Sort values along a given dim. Returns {values, indices} with indices int64.
std::pair<Series, Series> sort(const Series& a, std::int64_t dim, bool descending, bool stable);

// Argsort along dim. Returns indices int64.
Series argsort(const Series& a, std::int64_t dim, bool descending, bool stable);

// Top-k along dim. Returns {values, indices}. Indices int64.
std::pair<Series, Series> topk(const Series& a, std::int64_t k, std::int64_t dim, bool largest, bool sorted);

} // namespace pluss::codem::native::cpu
