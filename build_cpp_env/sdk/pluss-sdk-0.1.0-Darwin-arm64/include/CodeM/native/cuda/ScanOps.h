#pragma once

#include "CodeM/core/ScalarType.h"
#include "CodeM/core/SeriesSlim.h"

#include <cstdint>

namespace pluss::codem::native::cuda {

bool native_scan_enabled() noexcept;
bool native_scan_strict() noexcept;

bool supports_cumsum_scan_baseline(ScalarType dtype) noexcept;
bool supports_cumprod_scan_baseline(ScalarType dtype) noexcept;

bool supports_cumsum_scan(const Series& a, std::int64_t dim) noexcept;
bool supports_cumprod_scan(const Series& a, std::int64_t dim) noexcept;

} // namespace pluss::codem::native::cuda

