#pragma once

#include "CodeM/core/SeriesSlim.h"

#include <cstdint>
#include <utility>

namespace pluss::codem::native::mps {

bool supports_sort_selection(const Series& a, std::int64_t dim, bool stable) noexcept;
std::pair<Series, Series> sort_with_indices(const Series& a, std::int64_t dim, bool descending);
std::pair<Series, Series> topk_with_indices(const Series& a, std::int64_t k, std::int64_t dim, bool largest);
Series argsort(const Series& a, std::int64_t dim, bool descending);
Series kthvalue(const Series& a, std::int64_t k, std::int64_t dim, bool keepdim);

} // namespace pluss::codem::native::mps
