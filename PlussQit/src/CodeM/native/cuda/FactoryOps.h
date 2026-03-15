#pragma once

#include "CodeM/core/Series.h"

namespace pluss::codem::native::cuda {

bool native_factory_enabled() noexcept;
bool native_factory_strict() noexcept;

bool supports_zeros_factory_baseline(ScalarType dtype) noexcept;
bool supports_ones_factory_baseline(ScalarType dtype) noexcept;
bool supports_full_factory_baseline(ScalarType dtype) noexcept;
bool supports_arange_factory_baseline(ScalarType dtype) noexcept;
bool supports_linspace_factory_baseline(ScalarType dtype) noexcept;
bool supports_eye_factory_baseline(ScalarType dtype) noexcept;
bool supports_rand_factory_baseline(ScalarType dtype) noexcept;
bool supports_randn_factory_baseline(ScalarType dtype) noexcept;

} // namespace pluss::codem::native::cuda

