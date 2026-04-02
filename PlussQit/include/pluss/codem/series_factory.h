#pragma once

#include "pluss/export.h"
#include "pluss/config.h"
#include "pluss/codem/codem.h"
#include "pluss/codem/stack_array.h"
#include "pluss/codem/series_options.h"
#include "pluss/codem/series_value.h"
#include "pluss/detail/factory_core.h"
#include "pluss/detail/factory_containers.h"
#include "pluss/detail/series_finalize.h"
#include "CodeM/core/Config.h"
#include "pluss/detail/data_reader.h"

#include <cstdint>
#include <cmath>
#include <span>
#include <type_traits>
#include <vector>



#ifdef PLUSS_ENABLE_TORCH
#include <torch/torch.h>
#include <pluss/codem/torch_interop.h>
#endif

namespace pluss {
  // ---- Config builder helpers ----
  using pluss::config; // from pluss/config.h
  using detail::FactoryCore;

  // --------------------------------------------------------------------------
  // Factories (create NEW Series). Ops on existing Series live as methods.
  // --------------------------------------------------------------------------

  inline Series empty(std::span<const std::int64_t> sizes, ScalarType dt) { return Series::empty(sizes, dt); }
  inline Series empty(std::initializer_list<std::int64_t> sizes, ScalarType dt) {
    return empty(std::span<const std::int64_t>(sizes.begin(), sizes.size()), dt);
  }
  inline Series empty(std::span<const std::int64_t> sizes, Config cfg) { return Series::empty(sizes, cfg.dtype, cfg.platform); }
  inline Series empty(std::initializer_list<std::int64_t> sizes, Config cfg) {
    return empty(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg);
  }
  template <class... Opts>
  inline Series empty(std::span<const std::int64_t> sizes, Opts... opts) {
    return empty(sizes, FactoryCore::make_config(opts...));
  }
  template <class... Opts>
  inline Series empty(std::initializer_list<std::int64_t> sizes, Opts... opts) {
    return empty(std::span<const std::int64_t>(sizes.begin(), sizes.size()), FactoryCore::make_config(opts...));
  }
  template <class T>
  inline Series empty(std::span<const std::int64_t> sizes, std::initializer_list<std::pair<const char*, T>> kv) {
    return empty(sizes, pluss::config(kv));
  }

  inline Series zeros(std::span<const std::int64_t> sizes, ScalarType dt = ScalarType::Float32) { return Series::zeros(sizes, dt); }
  inline Series zeros(std::initializer_list<std::int64_t> sizes, ScalarType dt = ScalarType::Float32) {
    return zeros(std::span<const std::int64_t>(sizes.begin(), sizes.size()), dt);
  }
  inline Series zeros(std::span<const std::int64_t> sizes, Config cfg) {
    return FactoryCore::move_factory_to_config_platform_with_cuda_policy(
        Series::zeros(sizes, cfg.dtype),
        cfg,
        "zeros",
        pluss::cuda::supports_zeros_factory_baseline(cfg.dtype));
  }
  inline Series zeros(std::initializer_list<std::int64_t> sizes, Config cfg) {
    return zeros(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg);
  }
  template <class... Opts>
  inline Series zeros(std::span<const std::int64_t> sizes, Opts... opts) {
    return zeros(sizes, FactoryCore::make_config(opts...));
  }
  template <class... Opts>
  inline Series zeros(std::initializer_list<std::int64_t> sizes, Opts... opts) {
    return zeros(std::span<const std::int64_t>(sizes.begin(), sizes.size()), FactoryCore::make_config(opts...));
  }
  template <class T>
  inline Series zeros(std::span<const std::int64_t> sizes, std::initializer_list<std::pair<const char*, T>> kv) {
    return zeros(sizes, pluss::config(kv));
  }

  inline Series ones(std::span<const std::int64_t> sizes, ScalarType dt = ScalarType::Float32) { return Series::ones(sizes, dt); }
  inline Series ones(std::initializer_list<std::int64_t> sizes, ScalarType dt = ScalarType::Float32) {
    return ones(std::span<const std::int64_t>(sizes.begin(), sizes.size()), dt);
  }
  inline Series ones(std::span<const std::int64_t> sizes, Config cfg) {
    return FactoryCore::move_factory_to_config_platform_with_cuda_policy(
        Series::ones(sizes, cfg.dtype),
        cfg,
        "ones",
        pluss::cuda::supports_ones_factory_baseline(cfg.dtype));
  }
  inline Series ones(std::initializer_list<std::int64_t> sizes, Config cfg) {
    return ones(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg);
  }
  template <class... Opts>
  inline Series ones(std::span<const std::int64_t> sizes, Opts... opts) {
    return ones(sizes, FactoryCore::make_config(opts...));
  }
  template <class... Opts>
  inline Series ones(std::initializer_list<std::int64_t> sizes, Opts... opts) {
    return ones(std::span<const std::int64_t>(sizes.begin(), sizes.size()), FactoryCore::make_config(opts...));
  }
  template <class T>
  inline Series ones(std::span<const std::int64_t> sizes, std::initializer_list<std::pair<const char*, T>> kv) {
    return ones(sizes, pluss::config(kv));
  }

  inline Series full(std::span<const std::int64_t> sizes, double fill_value, ScalarType dt = ScalarType::Float32) { return Series::full(sizes, fill_value, dt); }
  inline Series full(std::initializer_list<std::int64_t> sizes, double fill_value, ScalarType dt = ScalarType::Float32) {
    return full(std::span<const std::int64_t>(sizes.begin(), sizes.size()), fill_value, dt);
  }
  inline Series full(std::span<const std::int64_t> sizes, double fill_value, Config cfg) {
    return FactoryCore::move_factory_to_config_platform_with_cuda_policy(
        Series::full(sizes, fill_value, cfg.dtype),
        cfg,
        "full",
        pluss::cuda::supports_full_factory_baseline(cfg.dtype));
  }
  inline Series full(std::initializer_list<std::int64_t> sizes, double fill_value, Config cfg) {
    return full(std::span<const std::int64_t>(sizes.begin(), sizes.size()), fill_value, cfg);
  }
  template <class... Opts>
  inline Series full(std::span<const std::int64_t> sizes, double fill_value, Opts... opts) {
    return full(sizes, fill_value, FactoryCore::make_config(opts...));
  }
  template <class... Opts>
  inline Series full(std::initializer_list<std::int64_t> sizes, double fill_value, Opts... opts) {
    return full(std::span<const std::int64_t>(sizes.begin(), sizes.size()), fill_value, FactoryCore::make_config(opts...));
  }
  template <class T>
  inline Series full(std::span<const std::int64_t> sizes, double fill_value, std::initializer_list<std::pair<const char*, T>> kv) {
    return full(sizes, fill_value, pluss::config(kv));
  }

  // Scalar constants (factories)
  inline Series nan(ScalarType dt = ScalarType::Float32) { return codem::nan(dt); }
  inline Series inf(ScalarType dt = ScalarType::Float32) { return codem::inf(dt); }

  // Ranges / random
  inline Series arange(std::int64_t end, ScalarType dt = ScalarType::Int64) { return Series::arange(0, end, 1, dt); }
  inline Series arange(std::int64_t start, std::int64_t end, ScalarType dt = ScalarType::Int64) { return Series::arange(start, end, 1, dt); }
  inline Series arange(std::int64_t start, std::int64_t end, std::int64_t step, ScalarType dt = ScalarType::Int64) { return Series::arange(start, end, step, dt); }
  inline Series arange(std::int64_t start, std::int64_t end, Config cfg) {
    ScalarType dt = cfg.dtype;
    if (dt == ScalarType::Float32) dt = ScalarType::Int64;
    return FactoryCore::move_factory_to_config_platform_with_cuda_policy(
        Series::arange(start, end, 1, dt),
        cfg,
        "arange",
        pluss::cuda::supports_arange_factory_baseline(dt));
  }
  inline Series arange(std::int64_t start, std::int64_t end, std::int64_t step, Config cfg) {
    ScalarType dt = cfg.dtype;
    if (dt == ScalarType::Float32) dt = ScalarType::Int64;
    return FactoryCore::move_factory_to_config_platform_with_cuda_policy(
        Series::arange(start, end, step, dt),
        cfg,
        "arange",
        pluss::cuda::supports_arange_factory_baseline(dt));
  }
  inline Series linspace(double start, double end, std::int64_t steps, ScalarType dt = ScalarType::Float32) { return Series::linspace(start, end, steps, dt); }
  inline Series linspace(double start, double end, std::int64_t steps, Config cfg) {
    return FactoryCore::move_factory_to_config_platform_with_cuda_policy(
        Series::linspace(start, end, steps, cfg.dtype),
        cfg,
        "linspace",
        pluss::cuda::supports_linspace_factory_baseline(cfg.dtype));
  }
  inline Series eye(std::int64_t n, std::int64_t m = -1, ScalarType dt = ScalarType::Float32) { return Series::eye(n, m, dt); }
  inline Series eye(std::int64_t n, Config cfg) {
    return FactoryCore::move_factory_to_config_platform_with_cuda_policy(
        Series::eye(n, -1, cfg.dtype),
        cfg,
        "eye",
        pluss::cuda::supports_eye_factory_baseline(cfg.dtype));
  }
  inline Series eye(std::int64_t n, std::int64_t m, Config cfg) {
    return FactoryCore::move_factory_to_config_platform_with_cuda_policy(
        Series::eye(n, m, cfg.dtype),
        cfg,
        "eye",
        pluss::cuda::supports_eye_factory_baseline(cfg.dtype));
  }

  inline Series rand(std::span<const std::int64_t> sizes, ScalarType dt = ScalarType::Float32, std::uint64_t seed = 0) { return Series::rand(sizes, dt, seed); }
  inline Series rand(std::initializer_list<std::int64_t> sizes, ScalarType dt = ScalarType::Float32, std::uint64_t seed = 0) {return Series::rand(std::span<const std::int64_t>(sizes.begin(), sizes.size()), dt, seed);}
  inline Series rand(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed = 0) {
    return FactoryCore::move_factory_to_config_platform_with_cuda_policy(
        Series::rand(sizes, cfg.dtype, seed),
        cfg,
        "rand",
        pluss::cuda::supports_rand_factory_baseline(cfg.dtype));
  }
  inline Series rand(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed = 0) {
    return rand(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
  }

  template <class... Opts>
  inline Series rand(std::span<const std::int64_t> sizes, std::uint64_t seed, Opts... opts) {
    return rand(sizes, FactoryCore::make_config(opts...), seed);
  }
  template <class... Opts>
  inline Series rand(std::initializer_list<std::int64_t> sizes, std::uint64_t seed, Opts... opts) {
    const Config cfg = FactoryCore::make_config(opts...);
    return rand(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
  }

  inline Series randn(std::span<const std::int64_t> sizes, ScalarType dt = ScalarType::Float32, std::uint64_t seed = 0) { return Series::randn(sizes, dt, seed); }
  inline Series randn(std::initializer_list<std::int64_t> sizes, ScalarType dt = ScalarType::Float32, std::uint64_t seed = 0) {
    return Series::randn(std::span<const std::int64_t>(sizes.begin(), sizes.size()), dt, seed);
  }
  inline Series randn(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed = 0) {
    return FactoryCore::move_factory_to_config_platform_with_cuda_policy(
        Series::randn(sizes, cfg.dtype, seed),
        cfg,
        "randn",
        pluss::cuda::supports_randn_factory_baseline(cfg.dtype));
  }
  inline Series randn(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed = 0) {
    return randn(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
  }
  template <class... Opts>
  inline Series randn(std::span<const std::int64_t> sizes, std::uint64_t seed, Opts... opts) {
    return randn(sizes, FactoryCore::make_config(opts...), seed);
  }
  template <class... Opts>
  inline Series randn(std::initializer_list<std::int64_t> sizes, std::uint64_t seed, Opts... opts) {
    const Config cfg = FactoryCore::make_config(opts...);
    return randn(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
  }

  inline Series randint(std::int64_t lo, std::int64_t hi, std::span<const std::int64_t> sizes, ScalarType dt = ScalarType::Int64, std::uint64_t seed = 0) {
    if (hi <= lo) pluss::codem::throw_error("randint: expected hi > lo");
    const double span = static_cast<double>(hi - lo);
    Series out = Series::rand(sizes, ScalarType::Float64, seed);
    out = out.mul(span).floor().add(static_cast<double>(lo));
    return out.to(dt);
  }
  inline Series randint(std::int64_t lo, std::int64_t hi, std::initializer_list<std::int64_t> sizes, ScalarType dt = ScalarType::Int64, std::uint64_t seed = 0) {
    return randint(lo, hi, std::span<const std::int64_t>(sizes.begin(), sizes.size()), dt, seed);
  }
  inline Series randint(std::int64_t lo, std::int64_t hi, std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed = 0) {
    ScalarType dt = cfg.dtype;
    if (dt == ScalarType::Float32) dt = ScalarType::Int64;
    return FactoryCore::move_factory_to_config_platform_with_cuda_policy(
        randint(lo, hi, sizes, dt, seed),
        cfg,
        "randint",
        pluss::cuda::supports_rand_factory_baseline(ScalarType::Float64));
  }
  inline Series randint(std::int64_t lo, std::int64_t hi, std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed = 0) {
    return randint(lo, hi, std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
  }
  template <class... Opts>
  inline Series randint(std::int64_t lo, std::int64_t hi, std::span<const std::int64_t> sizes, std::uint64_t seed, Opts... opts) {
    return randint(lo, hi, sizes, FactoryCore::make_config(opts...), seed);
  }
  template <class... Opts>
  inline Series randint(std::int64_t lo, std::int64_t hi, std::initializer_list<std::int64_t> sizes, std::uint64_t seed, Opts... opts) {
    return randint(lo, hi, std::span<const std::int64_t>(sizes.begin(), sizes.size()), FactoryCore::make_config(opts...), seed);
  }

  inline Series logspace(double start,
                         double end,
                         std::int64_t steps,
                         double base = 10.0,
                         ScalarType dt = ScalarType::Float32) {
    PLUSS_CHECK(steps > 0, "logspace: expected steps > 0");
    PLUSS_CHECK(base > 0.0 && base != 1.0, "logspace: expected base > 0 and base != 1");
    auto exponents = Series::linspace(start, end, steps, ScalarType::Float64);
    auto out = exponents.mul(std::log(base)).exp();
    return out.to(dt);
  }

  inline Series randperm(std::int64_t n,
                         ScalarType dt = ScalarType::Int64,
                         std::uint64_t seed = 0) {
    PLUSS_CHECK(n >= 0, "randperm: expected n >= 0");
    std::vector<std::int64_t> v(static_cast<std::size_t>(n));
    std::iota(v.begin(), v.end(), 0);
    auto rng = FactoryCore::make_rng(seed);
    std::shuffle(v.begin(), v.end(), rng);
    Series out = codem::make_series<std::int64_t>(
      v,
      std::span<const std::int64_t>(&n, 1),
      Config{}
    );
    return (dt == ScalarType::Int64) ? out : out.to(dt);
  }
  inline Series randperm(std::int64_t n, Config cfg, std::uint64_t seed = 0) {
    ScalarType dt = cfg.dtype;
    if (dt == ScalarType::Float32) dt = ScalarType::Int64;
    return FactoryCore::move_stochastic_factory_to_config_platform_with_cuda_policy(
        randperm(n, dt, seed),
        cfg,
        "randperm",
        pluss::cuda::supports_random_stochastic_baseline(dt));
  }

  inline Series normal(double mean,
                       double std,
                       std::span<const std::int64_t> sizes,
                       ScalarType dt = ScalarType::Float32,
                       std::uint64_t seed = 0) {
    PLUSS_CHECK(std >= 0.0, "normal: expected std >= 0");
    return Series::randn(sizes, dt, seed).mul(std).add(mean);
  }
  inline Series normal(double mean,
                       double std,
                       std::span<const std::int64_t> sizes,
                       Config cfg,
                       std::uint64_t seed = 0) {
    PLUSS_CHECK(std >= 0.0, "normal: expected std >= 0");
    return FactoryCore::move_stochastic_factory_to_config_platform_with_cuda_policy(
        Series::randn(sizes, cfg.dtype, seed).mul(std).add(mean),
        cfg,
        "normal",
        pluss::cuda::supports_normal_stochastic_baseline(cfg.dtype));
  }
  inline Series normal(double mean,
                       double std,
                       std::initializer_list<std::int64_t> sizes,
                       ScalarType dt = ScalarType::Float32,
                       std::uint64_t seed = 0) {
    return normal(mean, std, std::span<const std::int64_t>(sizes.begin(), sizes.size()), dt, seed);
  }
  inline Series normal(double mean,
                       double std,
                       std::initializer_list<std::int64_t> sizes,
                       Config cfg,
                       std::uint64_t seed = 0) {
    return normal(mean, std, std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
  }

  inline Series bernoulli(std::span<const std::int64_t> sizes,
                          double p = 0.5,
                          ScalarType dt = ScalarType::Bool,
                          std::uint64_t seed = 0) {
    PLUSS_CHECK(p >= 0.0 && p <= 1.0, "bernoulli: expected 0 <= p <= 1");
    std::int64_t n = 1;
    for (const auto s : sizes) {
      PLUSS_CHECK(s >= 0, "bernoulli: sizes must be >= 0");
      n *= s;
    }
    std::vector<std::uint8_t> bits(static_cast<std::size_t>(n), 0);
    auto rng = FactoryCore::make_rng(seed);
    std::bernoulli_distribution dist(p);
    for (std::int64_t i = 0; i < n; ++i) {
      bits[static_cast<std::size_t>(i)] = dist(rng) ? 1u : 0u;
    }
    Series out = codem::make_series<std::uint8_t>(bits, sizes, Config{}.with_dtype(ScalarType::Bool));
    return (dt == ScalarType::Bool) ? out : out.to(dt);
  }
  inline Series bernoulli(std::span<const std::int64_t> sizes,
                          double p,
                          Config cfg,
                          std::uint64_t seed = 0) {
    ScalarType dt = cfg.dtype;
    if (dt == ScalarType::Float32) dt = ScalarType::Bool;
    return FactoryCore::move_stochastic_factory_to_config_platform_with_cuda_policy(
        bernoulli(sizes, p, dt, seed),
        cfg,
        "bernoulli",
        pluss::cuda::supports_bernoulli_stochastic_baseline(dt));
  }
  inline Series bernoulli(std::initializer_list<std::int64_t> sizes,
                          double p = 0.5,
                          ScalarType dt = ScalarType::Bool,
                          std::uint64_t seed = 0) {
    return bernoulli(std::span<const std::int64_t>(sizes.begin(), sizes.size()), p, dt, seed);
  }
  inline Series bernoulli(std::initializer_list<std::int64_t> sizes,
                          double p,
                          Config cfg,
                          std::uint64_t seed = 0) {
    return bernoulli(std::span<const std::int64_t>(sizes.begin(), sizes.size()), p, cfg, seed);
  }

  inline Series poisson(std::span<const std::int64_t>, double, ScalarType = ScalarType::Int64, std::uint64_t = 0) {
    pluss::codem::throw_error("poisson factory: not implemented yet for CPU");
    return Series::undefined();
  }
  inline Series poisson(std::initializer_list<std::int64_t> sizes,
                        double lambda,
                        ScalarType dt = ScalarType::Int64,
                        std::uint64_t seed = 0) {
    return poisson(std::span<const std::int64_t>(sizes.begin(), sizes.size()), lambda, dt, seed);
  }

  inline Series multinomial(std::span<const double> probs,
                            std::int64_t num_samples,
                            bool replacement = true,
                            std::uint64_t seed = 0) {
    PLUSS_CHECK(!probs.empty(), "multinomial: probs cannot be empty");
    PLUSS_CHECK(num_samples >= 0, "multinomial: expected num_samples >= 0");
    std::vector<double> weights(probs.begin(), probs.end());
    for (const double w : weights) {
      PLUSS_CHECK(w >= 0.0, "multinomial: probs must be non-negative");
    }

    auto rng = FactoryCore::make_rng(seed);
    std::vector<std::int64_t> out(static_cast<std::size_t>(num_samples), 0);
    if (replacement) {
      std::discrete_distribution<std::int64_t> dist(weights.begin(), weights.end());
      for (std::int64_t i = 0; i < num_samples; ++i) {
        out[static_cast<std::size_t>(i)] = dist(rng);
      }
      return codem::make_series<std::int64_t>(
        out,
        std::span<const std::int64_t>(&num_samples, 1),
        Config{}
      );
    }

    std::vector<double> local = weights;
    for (std::int64_t i = 0; i < num_samples; ++i) {
      const double mass = std::accumulate(local.begin(), local.end(), 0.0);
      PLUSS_CHECK(mass > 0.0, "multinomial: not enough non-zero probability mass for sampling without replacement");
      std::discrete_distribution<std::int64_t> dist(local.begin(), local.end());
      const auto idx = dist(rng);
      out[static_cast<std::size_t>(i)] = idx;
      local[static_cast<std::size_t>(idx)] = 0.0;
    }
    return codem::make_series<std::int64_t>(
      out,
      std::span<const std::int64_t>(&num_samples, 1),
      Config{}
    );
  }

  inline Series multinomial(std::span<const double> probs,
                            std::int64_t num_samples,
                            bool replacement,
                            Config cfg,
                            std::uint64_t seed = 0) {
    const bool route_supported =
        pluss::cuda::supports_multinomial_stochastic_baseline(ScalarType::Float64) &&
        !probs.empty() &&
        num_samples >= 0 &&
        (replacement || num_samples <= static_cast<std::int64_t>(probs.size()));
    return FactoryCore::move_stochastic_factory_to_config_platform_with_cuda_policy(
        multinomial(probs, num_samples, replacement, seed),
        cfg,
        "multinomial",
        route_supported);
  }

  // Like-factories
  inline Series empty_like(const Series& like, ScalarType dt) {
    return empty(like.sizes(), Config{}.with_dtype(dt).with_platform(like.platform()));
  }
  inline Series empty_like(const Series& like, Config cfg = {}) {
    return FactoryCore::move_to_config_platform(empty_like(like, FactoryCore::resolve_like_dtype(like, cfg)), cfg);
  }
  template <class... Opts>
  inline Series empty_like(const Series& like, Opts... opts) {
    return empty_like(like, FactoryCore::make_config(opts...));
  }

  inline Series zeros_like(const Series& like, ScalarType dt) {
    return zeros(like.sizes(), Config{}.with_dtype(dt).with_platform(like.platform()));
  }
  inline Series zeros_like(const Series& like, Config cfg = {}) {
    return zeros(like.sizes(), cfg.with_dtype(FactoryCore::resolve_like_dtype(like, cfg)));
  }
  template <class... Opts>
  inline Series zeros_like(const Series& like, Opts... opts) {
    return zeros_like(like, FactoryCore::make_config(opts...));
  }
  inline Series zeroes_like(const Series& like, Config cfg = {}) { return zeros_like(like, cfg); }

  inline Series ones_like(const Series& like, ScalarType dt) {
    return ones(like.sizes(), Config{}.with_dtype(dt).with_platform(like.platform()));
  }
  inline Series ones_like(const Series& like, Config cfg = {}) {
    return ones(like.sizes(), cfg.with_dtype(FactoryCore::resolve_like_dtype(like, cfg)));
  }
  template <class... Opts>
  inline Series ones_like(const Series& like, Opts... opts) {
    return ones_like(like, FactoryCore::make_config(opts...));
  }

  inline Series full_like(const Series& like, double fill_value, ScalarType dt) {
    return full(like.sizes(), fill_value, Config{}.with_dtype(dt).with_platform(like.platform()));
  }
  inline Series full_like(const Series& like, double fill_value, Config cfg = {}) {
    return full(
        like.sizes(),
        fill_value,
        cfg.with_dtype(FactoryCore::resolve_like_dtype(like, cfg)));
  }
  template <class... Opts>
  inline Series full_like(const Series& like, double fill_value, Opts... opts) {
    return full_like(like, fill_value, FactoryCore::make_config(opts...));
  }

  inline Series rand_like(const Series& like, ScalarType dt, std::uint64_t seed = 0) {
    return pluss::rand(like.sizes(), Config{}.with_dtype(dt).with_platform(like.platform()), seed);
  }
  inline Series rand_like(const Series& like, Config cfg = {}, std::uint64_t seed = 0) {
    return rand(like.sizes(), cfg.with_dtype(FactoryCore::resolve_like_dtype(like, cfg)), seed);
  }

  inline Series randn_like(const Series& like, ScalarType dt, std::uint64_t seed = 0) {
    return pluss::randn(like.sizes(), Config{}.with_dtype(dt).with_platform(like.platform()), seed);
  }
  inline Series randn_like(const Series& like, Config cfg = {}, std::uint64_t seed = 0) {
    return randn(like.sizes(), cfg.with_dtype(FactoryCore::resolve_like_dtype(like, cfg)), seed);
  }

  inline Series randint_like(const Series& like,
                             std::int64_t hi,
                             ScalarType dt = ScalarType::Int64,
                             std::uint64_t seed = 0) {
    return randint(
        /*lo=*/0,
        hi,
        like.sizes(),
        Config{}.with_dtype(dt).with_platform(like.platform()),
        seed);
  }
  inline Series randint_like(const Series& like,
                             std::int64_t lo,
                             std::int64_t hi,
                             ScalarType dt = ScalarType::Int64,
                             std::uint64_t seed = 0) {
    return randint(
        lo,
        hi,
        like.sizes(),
        Config{}.with_dtype(dt).with_platform(like.platform()),
        seed);
  }
  inline Series randint_like(const Series& like,
                             std::int64_t hi,
                             Config cfg,
                             std::uint64_t seed = 0) {
    return randint(
        /*lo=*/0,
        hi,
        like.sizes(),
        cfg.with_dtype(FactoryCore::resolve_like_dtype(like, cfg)),
        seed);
  }
  inline Series randint_like(const Series& like,
                             std::int64_t lo,
                             std::int64_t hi,
                             Config cfg,
                             std::uint64_t seed = 0) {
    return randint(
        lo,
        hi,
        like.sizes(),
        cfg.with_dtype(FactoryCore::resolve_like_dtype(like, cfg)),
        seed);
  }

  inline Series eye_like(const Series& like, ScalarType dt = ScalarType::Float32) {
    PLUSS_CHECK(like.dim() >= 2, "eye_like: expected input with rank >= 2");
    const auto n = like.sizes()[static_cast<std::size_t>(like.dim() - 2)];
    const auto m = like.sizes()[static_cast<std::size_t>(like.dim() - 1)];
    return pluss::eye(n, m, Config{}.with_dtype(dt).with_platform(like.platform()));
  }
  inline Series eye_like(const Series& like, Config cfg) {
    PLUSS_CHECK(like.dim() >= 2, "eye_like: expected input with rank >= 2");
    const auto n = like.sizes()[static_cast<std::size_t>(like.dim() - 2)];
    const auto m = like.sizes()[static_cast<std::size_t>(like.dim() - 1)];
    return eye(n, m, cfg.with_dtype(FactoryCore::resolve_like_dtype(like, cfg)));
  }
  template <class... Opts>
  inline Series eye_like(const Series& like, Opts... opts) {
    return eye_like(like, FactoryCore::make_config(opts...));
  }

  inline Series complex(const Series& real,
                        const Series& imag,
                        ScalarType dt = ScalarType::Complex64) {
    PLUSS_CHECK(dt == ScalarType::Complex64 || dt == ScalarType::Complex128,
                "complex: dtype must be Complex64 or Complex128");
    PLUSS_CHECK(FactoryCore::same_shape(real.sizes(), imag.sizes()),
                "complex: real and imag must have the same shape");

    const ScalarType base_dt = (dt == ScalarType::Complex128) ? ScalarType::Float64 : ScalarType::Float32;
    auto rr = real.to(base_dt).contiguous();
    auto ii = imag.to(base_dt).contiguous();
    auto out = Series::empty(real.sizes(), dt);
    const auto n = out.numel();

    if (dt == ScalarType::Complex64) {
      const auto* rp = reinterpret_cast<const float*>(rr.raw_data());
      const auto* ip = reinterpret_cast<const float*>(ii.raw_data());
      auto* op = reinterpret_cast<codem::Complex64*>(out.raw_mutable_data());
      for (std::int64_t i = 0; i < n; ++i) {
        op[i] = codem::Complex64(rp[i], ip[i]);
      }
      return out;
    }

    const auto* rp = reinterpret_cast<const double*>(rr.raw_data());
    const auto* ip = reinterpret_cast<const double*>(ii.raw_data());
    auto* op = reinterpret_cast<codem::Complex128*>(out.raw_mutable_data());
    for (std::int64_t i = 0; i < n; ++i) {
      op[i] = codem::Complex128(rp[i], ip[i]);
    }
    return out;
  }

  inline Series polar(const Series& abs,
                      const Series& angle,
                      ScalarType dt = ScalarType::Complex64) {
    PLUSS_CHECK(FactoryCore::same_shape(abs.sizes(), angle.sizes()),
                "polar: abs and angle must have the same shape");
    PLUSS_CHECK(dt == ScalarType::Complex64 || dt == ScalarType::Complex128,
                "polar: dtype must be Complex64 or Complex128");
    const ScalarType base_dt = (dt == ScalarType::Complex128) ? ScalarType::Float64 : ScalarType::Float32;
    auto mag = abs.to(base_dt).contiguous();
    auto ang = angle.to(base_dt).contiguous();
    auto out = Series::empty(abs.sizes(), dt);
    const auto n = out.numel();

    if (dt == ScalarType::Complex64) {
      const auto* mp = reinterpret_cast<const float*>(mag.raw_data());
      const auto* ap = reinterpret_cast<const float*>(ang.raw_data());
      auto* op = reinterpret_cast<codem::Complex64*>(out.raw_mutable_data());
      for (std::int64_t i = 0; i < n; ++i) {
        const float m = mp[i];
        const float a = ap[i];
        op[i] = codem::Complex64(m * std::cos(a), m * std::sin(a));
      }
      return out;
    }

    const auto* mp = reinterpret_cast<const double*>(mag.raw_data());
    const auto* ap = reinterpret_cast<const double*>(ang.raw_data());
    auto* op = reinterpret_cast<codem::Complex128*>(out.raw_mutable_data());
    for (std::int64_t i = 0; i < n; ++i) {
      const double m = mp[i];
      const double a = ap[i];
      op[i] = codem::Complex128(m * std::cos(a), m * std::sin(a));
    }
    return out;
  }

  // Explicit API stubs for currently unsupported sparse/quantized constructors.
  template <class... Args>
  inline Series sparse_coo_tensor(Args&&...) {
    pluss::codem::throw_error("sparse_coo_tensor: not implemented yet for CPU");
    return Series::undefined();
  }
  template <class... Args>
  inline Series sparse_csr_tensor(Args&&...) {
    pluss::codem::throw_error("sparse_csr_tensor: not implemented yet for CPU");
    return Series::undefined();
  }
  template <class... Args>
  inline Series sparse_csc_tensor(Args&&...) {
    pluss::codem::throw_error("sparse_csc_tensor: not implemented yet for CPU");
    return Series::undefined();
  }
  template <class... Args>
  inline Series sparse_bsr_tensor(Args&&...) {
    pluss::codem::throw_error("sparse_bsr_tensor: not implemented yet for CPU");
    return Series::undefined();
  }
  template <class... Args>
  inline Series sparse_bsc_tensor(Args&&...) {
    pluss::codem::throw_error("sparse_bsc_tensor: not implemented yet for CPU");
    return Series::undefined();
  }
  template <class... Args>
  inline Series quantize_per_tensor(Args&&...) {
    pluss::codem::throw_error("quantize_per_tensor: not implemented yet for CPU");
    return Series::undefined();
  }
  template <class... Args>
  inline Series quantize_per_channel(Args&&...) {
    pluss::codem::throw_error("quantize_per_channel: not implemented yet for CPU");
    return Series::undefined();
  }

// interop with other libraries (e.g., Torch)
#ifdef PLUSS_ENABLE_TORCH
  inline Series from_torch(const ::torch::Tensor& t) { return codem::from_torch(t); }  // use to create a pluss::Series from a torch::Tensor (cheap copy of data)
  inline torch::Tensor to_torch(const Series& s) { return codem::to_torch(s); }
#endif

  // --------------------------------------------------------------------------
  // Value / blob / make_series factories (create NEW Series from user data)
  // --------------------------------------------------------------------------
  // Owned-construction entrypoint.
  // Data forms are parsed by DataReader constructors; options are applied here.
  template <class... Opts>
  inline Series make_series(detail::DataReader data,
                            detail::DataReader::Option first = {},
                            Opts&&... opts) {
    return codem::value(
        data,
        std::move(first),
        std::forward<Opts>(opts)...);
  }

  inline Series from_buffer(void* data,
                             std::span<const std::int64_t> sizes,
                             std::span<const std::int64_t> strides,
                             BlobDeleter deleter,
                             ScalarType dtype = ScalarType::Float32) {
    return codem::from_buffer(data, sizes, strides, deleter, dtype);
  }

  inline pluss::Series pile(StackArray<Series> values, std::int64_t dim = 0) {
    return pile(values.data(), dim);
  }

  inline pluss::Series unite(StackArray<Series> values, std::int64_t dim = 0, bool strict_dtype = true) {
    return unite(values.data(), dim, strict_dtype);
  }

} // namespace pluss
