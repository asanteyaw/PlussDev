#pragma once

#include "CodeM/core/ScalarType.h"
#include "CodeM/core/SeriesSlim.h"

namespace pluss::codem::native::cuda {

bool native_matmul_enabled() noexcept;
bool native_matmul_strict() noexcept;

bool supports_matmul_baseline(ScalarType lhs_dtype, ScalarType rhs_dtype) noexcept;
bool supports_matmul(const Series& a, const Series& b) noexcept;

} // namespace pluss::codem::native::cuda
