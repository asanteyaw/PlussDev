#pragma once

#include "CodeM/core/ScalarType.h"
#include "CodeM/core/SeriesSlim.h"

#include <cstdint>

namespace pluss::codem::native::cuda {

bool native_reduction_enabled() noexcept;
bool native_reduction_strict() noexcept;

bool supports_sum_reduction_baseline(ScalarType dtype) noexcept;
bool supports_prod_reduction_baseline(ScalarType dtype) noexcept;
bool supports_mean_reduction_baseline(ScalarType dtype) noexcept;
bool supports_ordered_extrema_reduction_baseline(ScalarType dtype) noexcept;
bool supports_logical_reduction_baseline(ScalarType dtype) noexcept;
bool supports_arg_extrema_reduction_baseline(ScalarType dtype) noexcept;

bool supports_sum_reduction(const Series& a) noexcept;
bool supports_sum_reduction(const Series& a, std::int64_t dim) noexcept;
bool supports_prod_reduction(const Series& a) noexcept;
bool supports_prod_reduction(const Series& a, std::int64_t dim) noexcept;
bool supports_mean_reduction(const Series& a) noexcept;
bool supports_mean_reduction(const Series& a, std::int64_t dim) noexcept;
bool supports_ordered_extrema_reduction(const Series& a) noexcept;
bool supports_ordered_extrema_reduction(const Series& a, std::int64_t dim) noexcept;
bool supports_logical_reduction(const Series& a) noexcept;
bool supports_logical_reduction(const Series& a, std::int64_t dim) noexcept;
bool supports_arg_extrema_reduction(const Series& a) noexcept;
bool supports_arg_extrema_reduction(const Series& a, std::int64_t dim) noexcept;

bool run_sum_reduction(const Series& a, Series& out) noexcept;
bool run_sum_reduction(const Series& a, std::int64_t dim, Series& out) noexcept;
bool run_prod_reduction(const Series& a, Series& out) noexcept;
bool run_prod_reduction(const Series& a, std::int64_t dim, Series& out) noexcept;
bool run_mean_reduction(const Series& a, Series& out) noexcept;
bool run_mean_reduction(const Series& a, std::int64_t dim, Series& out) noexcept;
bool run_amax_reduction(const Series& a, Series& out) noexcept;
bool run_amax_reduction(const Series& a, std::int64_t dim, Series& out) noexcept;
bool run_amin_reduction(const Series& a, Series& out) noexcept;
bool run_amin_reduction(const Series& a, std::int64_t dim, Series& out) noexcept;
bool run_all_reduction(const Series& a, Series& out) noexcept;
bool run_all_reduction(const Series& a, std::int64_t dim, Series& out) noexcept;
bool run_any_reduction(const Series& a, Series& out) noexcept;
bool run_any_reduction(const Series& a, std::int64_t dim, Series& out) noexcept;
bool run_argmax_reduction(const Series& a, std::int64_t dim, Series& out) noexcept;
bool run_argmin_reduction(const Series& a, std::int64_t dim, Series& out) noexcept;

} // namespace pluss::codem::native::cuda
