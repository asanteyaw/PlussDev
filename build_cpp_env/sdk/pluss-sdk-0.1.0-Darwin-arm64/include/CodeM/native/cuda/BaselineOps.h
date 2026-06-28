#pragma once

#include "CodeM/core/ScalarType.h"

namespace pluss::codem::native::cuda {

bool native_baseline_enabled() noexcept;
bool supports_elementwise_lane(ScalarType dtype) noexcept;
bool supports_reduction_lane(ScalarType dtype) noexcept;
bool supports_matmul_lane(ScalarType lhs_dtype, ScalarType rhs_dtype) noexcept;

} // namespace pluss::codem::native::cuda
