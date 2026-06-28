#pragma once

#include "CodeM/core/ScalarType.h"
#include "CodeM/core/SeriesSlim.h"

namespace pluss::codem::native::cuda {

bool native_bitwise_enabled() noexcept;
bool native_bitwise_strict() noexcept;

bool supports_bitwise_unary_baseline(ScalarType dtype) noexcept;
bool supports_bitwise_binary_baseline(ScalarType lhs_dtype, ScalarType rhs_dtype) noexcept;

bool supports_bitwise_unary(const Series& a) noexcept;
bool supports_bitwise_binary(const Series& a, const Series& b) noexcept;

} // namespace pluss::codem::native::cuda

