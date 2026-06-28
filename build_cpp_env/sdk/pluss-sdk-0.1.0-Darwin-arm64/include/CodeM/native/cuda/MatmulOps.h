#pragma once

#include "CodeM/core/ScalarType.h"
#include "CodeM/core/SeriesSlim.h"

#include <optional>
#include <string>

namespace pluss::codem::native::cuda {

bool native_matmul_enabled() noexcept;
bool native_matmul_strict() noexcept;

bool supports_matmul_baseline(ScalarType lhs_dtype, ScalarType rhs_dtype) noexcept;
bool supports_matmul(const Series& a, const Series& b) noexcept;

bool cublas_matmul_enabled() noexcept;
bool cublas_matmul_strict() noexcept;
bool cublas_matmul_ready() noexcept;

bool supports_cublas_matmul(
    const Series& a,
    const Series& b,
    std::string* reason = nullptr) noexcept;

std::optional<Series> try_cublas_matmul(const Series& a, const Series& b);

} // namespace pluss::codem::native::cuda
