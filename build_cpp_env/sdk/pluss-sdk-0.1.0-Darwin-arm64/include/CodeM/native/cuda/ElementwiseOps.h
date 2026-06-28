#pragma once

#include "CodeM/core/ScalarType.h"
#include "CodeM/core/SeriesSlim.h"
#include "CodeM/native/cpu/KernelTable.h"

namespace pluss::codem::native::cuda {

bool native_elementwise_enabled() noexcept;
bool native_elementwise_strict() noexcept;

bool supports_unary_elementwise_baseline(ScalarType dtype) noexcept;
bool supports_binary_elementwise_baseline(ScalarType lhs_dtype, ScalarType rhs_dtype) noexcept;

bool supports_unary_elementwise(const Series& a) noexcept;
bool supports_binary_elementwise(const Series& a, const Series& b) noexcept;
bool supports_comparison_elementwise(const Series& a, const Series& b) noexcept;
bool supports_where_elementwise(const Series& cond, const Series& a, const Series& b) noexcept;

bool run_unary_elementwise(native::cpu::UnaryOpId op, const Series& a, Series& out) noexcept;
bool run_binary_elementwise(native::cpu::BinaryOpId op, const Series& a, const Series& b, Series& out) noexcept;
bool run_where_elementwise(const Series& cond, const Series& a, const Series& b, Series& out) noexcept;

} // namespace pluss::codem::native::cuda
