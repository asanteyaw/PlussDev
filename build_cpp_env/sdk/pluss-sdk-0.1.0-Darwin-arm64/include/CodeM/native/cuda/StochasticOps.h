#pragma once

#include "CodeM/core/Series.h"

#include <cstdint>

namespace pluss::codem::native::cuda {

bool native_stochastic_enabled() noexcept;
bool native_stochastic_strict() noexcept;

bool supports_bernoulli_stochastic_baseline(ScalarType dtype) noexcept;
bool supports_normal_stochastic_baseline(ScalarType dtype) noexcept;
bool supports_log_normal_stochastic_baseline(ScalarType dtype) noexcept;
bool supports_uniform_stochastic_baseline(ScalarType dtype) noexcept;
bool supports_exponential_stochastic_baseline(ScalarType dtype) noexcept;
bool supports_geometric_stochastic_baseline(ScalarType dtype) noexcept;
bool supports_cauchy_stochastic_baseline(ScalarType dtype) noexcept;
bool supports_random_stochastic_baseline(ScalarType dtype) noexcept;
bool supports_multinomial_stochastic_baseline(ScalarType dtype) noexcept;
bool supports_poisson_stochastic_baseline(ScalarType dtype) noexcept;

bool supports_bernoulli_stochastic(const Series& a) noexcept;
bool supports_normal_stochastic(const Series& a) noexcept;
bool supports_log_normal_stochastic(const Series& a) noexcept;
bool supports_uniform_stochastic(const Series& a) noexcept;
bool supports_exponential_stochastic(const Series& a) noexcept;
bool supports_geometric_stochastic(const Series& a) noexcept;
bool supports_cauchy_stochastic(const Series& a) noexcept;
bool supports_random_stochastic(const Series& a) noexcept;
bool supports_multinomial_stochastic(const Series& a, std::int64_t num_samples, bool replacement) noexcept;
bool supports_poisson_stochastic(ScalarType dtype) noexcept;

} // namespace pluss::codem::native::cuda
