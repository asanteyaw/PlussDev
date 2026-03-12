#pragma once

#include "CodeM/core/ScalarType.h"
#include "CodeM/core/SeriesSlim.h"

namespace pluss::codem::native::cuda {

bool native_elementwise_enabled() noexcept;
bool native_elementwise_strict() noexcept;

bool supports_unary_elementwise_baseline(ScalarType dtype) noexcept;
bool supports_binary_elementwise_baseline(ScalarType lhs_dtype, ScalarType rhs_dtype) noexcept;

bool supports_unary_elementwise(const Series& a) noexcept;
bool supports_binary_elementwise(const Series& a, const Series& b) noexcept;
bool supports_comparison_elementwise(const Series& a, const Series& b) noexcept;

} // namespace pluss::codem::native::cuda
