#pragma once

#include "CodeM/core/ScalarType.h"
#include "CodeM/core/SeriesSlim.h"

namespace pluss::codem::native::cuda {

bool native_counting_enabled() noexcept;
bool native_counting_strict() noexcept;

bool supports_unique_counting_baseline(ScalarType dtype, bool sorted) noexcept;
bool supports_bincount_counting_baseline(ScalarType dtype) noexcept;
bool supports_histc_counting_baseline(ScalarType dtype) noexcept;

bool supports_unique_counting(const Series& a, bool sorted) noexcept;
bool supports_bincount_counting(const Series& a, std::int64_t minlength) noexcept;
bool supports_histc_counting(const Series& a, std::int64_t bins) noexcept;

} // namespace pluss::codem::native::cuda
