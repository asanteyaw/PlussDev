#pragma once

#include "CodeM/core/ScalarType.h"
#include "CodeM/core/SeriesSlim.h"

#include <vector>

namespace pluss::codem::native::cuda {

bool native_indexing_enabled() noexcept;
bool native_indexing_strict() noexcept;

bool supports_take_put_baseline(ScalarType value_dtype, ScalarType index_dtype) noexcept;
bool supports_nonzero_baseline(ScalarType value_dtype) noexcept;
bool supports_index_baseline(ScalarType value_dtype, ScalarType index_dtype) noexcept;

bool supports_take(const Series& self, const Series& index) noexcept;
bool supports_put(const Series& self, const Series& index, const Series& source, bool accumulate) noexcept;
bool supports_nonzero(const Series& self) noexcept;
bool supports_index(const Series& self, const std::vector<Series>& indices) noexcept;

} // namespace pluss::codem::native::cuda
