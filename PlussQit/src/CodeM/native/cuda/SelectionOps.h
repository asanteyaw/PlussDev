#pragma once

#include "CodeM/core/ScalarType.h"
#include "CodeM/core/SeriesSlim.h"

#include <cstdint>

namespace pluss::codem::native::cuda {

bool native_selection_enabled() noexcept;
bool native_selection_strict() noexcept;

bool supports_sort_selection_baseline(ScalarType dtype, bool stable) noexcept;
bool supports_topk_selection_baseline(ScalarType dtype, bool sorted) noexcept;
bool supports_kthvalue_selection_baseline(ScalarType dtype) noexcept;
bool supports_argsort_selection_baseline(ScalarType dtype, bool stable) noexcept;

bool supports_sort_selection(const Series& a, std::int64_t dim, bool stable) noexcept;
bool supports_topk_selection(const Series& a, std::int64_t k, std::int64_t dim, bool sorted) noexcept;
bool supports_kthvalue_selection(const Series& a, std::int64_t k, std::int64_t dim) noexcept;
bool supports_argsort_selection(const Series& a, std::int64_t dim, bool stable) noexcept;

} // namespace pluss::codem::native::cuda
