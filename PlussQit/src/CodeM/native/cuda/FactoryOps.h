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

bool run_fill_factory(Series& out, double value) noexcept;
bool run_arange_factory(Series& out, double start, double step) noexcept;
bool run_linspace_factory(Series& out, double start, double end) noexcept;
bool run_eye_factory(Series& out, std::int64_t n, std::int64_t m) noexcept;
bool run_uniform_factory(Series& out, std::uint64_t seed, double lo, double hi) noexcept;
bool run_normal_factory(Series& out, std::uint64_t seed, double mean, double stddev) noexcept;
bool run_randint_factory(Series& out, std::uint64_t seed, std::int64_t lo, std::int64_t hi) noexcept;
bool run_bernoulli_factory(Series& out, std::uint64_t seed, double p) noexcept;
bool run_randperm_factory(Series& out, std::uint64_t seed) noexcept;
bool run_complex_factory(const Series& real, const Series& imag, Series& out) noexcept;
bool run_polar_factory(const Series& abs, const Series& angle, Series& out) noexcept;

} // namespace pluss::codem::native::cuda
