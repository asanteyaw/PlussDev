#pragma once

#include "CodeM/core/ScalarType.h"
#include "CodeM/core/SeriesSlim.h"

namespace pluss::codem::native::cuda {

bool native_masking_enabled() noexcept;
bool native_masking_strict() noexcept;

bool supports_masked_select_baseline(ScalarType value_dtype, ScalarType mask_dtype) noexcept;
bool supports_masked_fill_baseline(ScalarType value_dtype, ScalarType mask_dtype) noexcept;
bool supports_masked_scatter_baseline(
    ScalarType value_dtype,
    ScalarType mask_dtype,
    ScalarType src_dtype) noexcept;

bool supports_masked_select(const Series& value, const Series& mask) noexcept;
bool supports_masked_fill(const Series& value, const Series& mask, const Series& fill_value) noexcept;
bool supports_masked_scatter(const Series& value, const Series& mask, const Series& src) noexcept;

} // namespace pluss::codem::native::cuda

