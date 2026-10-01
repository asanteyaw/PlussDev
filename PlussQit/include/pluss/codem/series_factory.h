#pragma once

#include "pluss/export.h"
#include "pluss/config.h"
#include "pluss/codem/codem.h"
#include "pluss/codem/stack_array.h"
#include "pluss/codem/series_options.h"
#include "pluss/codem/series_value.h"
#include "pluss/detail/factory_core.h"
#include "pluss/detail/factory_containers.h"
#include "pluss/detail/select_core.h"
#include "pluss/detail/series_finalize.h"
#include "MSharp/RangeView.h"
#include "CodeM/core/Config.h"
#include "CodeM/native/cuda/FactoryOps.h"
#include "CodeM/native/mps/Stochastic.h"
#include "pluss/detail/data_reader.h"

#include <cstdint>
#include <cmath>
#include <limits>
#include <random>
#include <span>
#include <type_traits>
#include <vector>



#ifdef PLUSS_ENABLE_TORCH
#include <torch/torch.h>
#include <pluss/codem/torch_interop.h>
#endif

// `Series` (capital S) names the type; `pluss::series::` (lowercase) is its
// dedicated factory namespace, kept separate from the type itself the same
// way `pluss::timeline::make`/`pluss::duration::*`/`pluss::note::*` are kept
// separate from `Timeline`/`Duration`/`Note`. Unlike those three, Series'
// value-construction entrypoint (`make`, formerly the flat `make_series`)
// already funnels through a single `DataReader`-based overload, so no new
// reader type was needed here -- this is a namespace relocation, not an
// overload-collapse.
//
// The old flat `pluss::` spellings (`pluss::zeros`, `pluss::make_series`,
// etc.) remain available -- see series_factory_compat.h, included at the
// bottom of this file -- since Series' flat names are used across the
// entire codebase and are not slated for a hard-rename sweep.
namespace pluss::series {
  // ---- Config builder helpers ----
  using pluss::config; // from pluss/config.h
  using detail::FactoryCore;

  // --------------------------------------------------------------------------
  // Factories (create NEW Series). Ops on existing Series live as methods.
  // --------------------------------------------------------------------------

  inline Series empty(pluss::msharp::RangeView::Int sizes, ScalarType dt) { return Series::empty(sizes, dt); }
  inline Series empty(std::initializer_list<std::int64_t> sizes, ScalarType dt) {
    return empty(pluss::msharp::RangeView::Int(sizes), dt);
  }
  inline Series empty(pluss::msharp::RangeView::Int sizes, Config cfg) { return Series::empty(sizes, cfg.dtype, cfg.platform); }
  inline Series empty(std::initializer_list<std::int64_t> sizes, Config cfg) {
    return empty(pluss::msharp::RangeView::Int(sizes), cfg);
  }
  template <class... Opts>
  inline Series empty(pluss::msharp::RangeView::Int sizes, Opts... opts) {
    return empty(sizes, FactoryCore::make_config(opts...));
  }
  template <class... Opts>
  inline Series empty(std::initializer_list<std::int64_t> sizes, Opts... opts) {
    return empty(pluss::msharp::RangeView::Int(sizes), FactoryCore::make_config(opts...));
  }
  template <class T>
  inline Series empty(pluss::msharp::RangeView::Int sizes, std::initializer_list<std::pair<const char*, T>> kv) {
    return empty(sizes, pluss::config(kv));
  }

  inline Series zeros(pluss::msharp::RangeView::Int sizes, ScalarType dt = ScalarType::Float32) { return Series::zeros(sizes, dt); }
  inline Series zeros(std::initializer_list<std::int64_t> sizes, ScalarType dt = ScalarType::Float32) {
    return zeros(pluss::msharp::RangeView::Int(sizes), dt);
  }
  inline Series zeros(pluss::msharp::RangeView::Int sizes, Config cfg) {
    const bool route_supported = pluss::cuda::supports_zeros_factory_baseline(cfg.dtype);
    if (FactoryCore::should_construct_native_cuda_factory(cfg, route_supported)) {
      return Series::zeros(sizes, cfg.dtype, Platform::CUDA);
    }
    FactoryCore::check_cuda_factory_host_staging_allowed(cfg, "zeros", route_supported);
    return FactoryCore::move_factory_to_config_platform_with_cuda_policy(
        Series::zeros(sizes, cfg.dtype),
        cfg,
        "zeros",
        route_supported);
  }
  inline Series zeros(std::initializer_list<std::int64_t> sizes, Config cfg) {
    return zeros(pluss::msharp::RangeView::Int(sizes), cfg);
  }
  template <class... Opts>
  inline Series zeros(pluss::msharp::RangeView::Int sizes, Opts... opts) {
    return zeros(sizes, FactoryCore::make_config(opts...));
  }
  template <class... Opts>
  inline Series zeros(std::initializer_list<std::int64_t> sizes, Opts... opts) {
    return zeros(pluss::msharp::RangeView::Int(sizes), FactoryCore::make_config(opts...));
  }
  template <class T>
  inline Series zeros(pluss::msharp::RangeView::Int sizes, std::initializer_list<std::pair<const char*, T>> kv) {
    return zeros(sizes, pluss::config(kv));
  }

  inline Series ones(pluss::msharp::RangeView::Int sizes, ScalarType dt = ScalarType::Float32) { return Series::ones(sizes, dt); }
  inline Series ones(std::initializer_list<std::int64_t> sizes, ScalarType dt = ScalarType::Float32) {
    return ones(pluss::msharp::RangeView::Int(sizes), dt);
  }
  inline Series ones(pluss::msharp::RangeView::Int sizes, Config cfg) {
    const bool route_supported = pluss::cuda::supports_ones_factory_baseline(cfg.dtype);
    if (FactoryCore::should_construct_native_cuda_factory(cfg, route_supported)) {
      return Series::ones(sizes, cfg.dtype, Platform::CUDA);
    }
    FactoryCore::check_cuda_factory_host_staging_allowed(cfg, "ones", route_supported);
    return FactoryCore::move_factory_to_config_platform_with_cuda_policy(
        Series::ones(sizes, cfg.dtype),
        cfg,
        "ones",
        route_supported);
  }
  inline Series ones(std::initializer_list<std::int64_t> sizes, Config cfg) {
    return ones(pluss::msharp::RangeView::Int(sizes), cfg);
  }
  template <class... Opts>
  inline Series ones(pluss::msharp::RangeView::Int sizes, Opts... opts) {
    return ones(sizes, FactoryCore::make_config(opts...));
  }
  template <class... Opts>
  inline Series ones(std::initializer_list<std::int64_t> sizes, Opts... opts) {
    return ones(pluss::msharp::RangeView::Int(sizes), FactoryCore::make_config(opts...));
  }
  template <class T>
  inline Series ones(pluss::msharp::RangeView::Int sizes, std::initializer_list<std::pair<const char*, T>> kv) {
    return ones(sizes, pluss::config(kv));
  }

  inline Series full(pluss::msharp::RangeView::Int sizes, double fill_value, ScalarType dt = ScalarType::Float32) { return Series::full(sizes, fill_value, dt); }
  inline Series full(std::initializer_list<std::int64_t> sizes, double fill_value, ScalarType dt = ScalarType::Float32) {
    return full(pluss::msharp::RangeView::Int(sizes), fill_value, dt);
  }
  inline Series full(pluss::msharp::RangeView::Int sizes, double fill_value, Config cfg) {
    const bool route_supported = pluss::cuda::supports_full_factory_baseline(cfg.dtype);
    if (FactoryCore::should_construct_native_cuda_factory(cfg, route_supported)) {
      return Series::full(sizes, fill_value, cfg.dtype, Platform::CUDA);
    }
    FactoryCore::check_cuda_factory_host_staging_allowed(cfg, "full", route_supported);
    return FactoryCore::move_factory_to_config_platform_with_cuda_policy(
        Series::full(sizes, fill_value, cfg.dtype),
        cfg,
        "full",
        route_supported);
  }
  inline Series full(std::initializer_list<std::int64_t> sizes, double fill_value, Config cfg) {
    return full(pluss::msharp::RangeView::Int(sizes), fill_value, cfg);
  }
  template <class... Opts>
  inline Series full(pluss::msharp::RangeView::Int sizes, double fill_value, Opts... opts) {
    return full(sizes, fill_value, FactoryCore::make_config(opts...));
  }
  template <class... Opts>
  inline Series full(std::initializer_list<std::int64_t> sizes, double fill_value, Opts... opts) {
    return full(pluss::msharp::RangeView::Int(sizes), fill_value, FactoryCore::make_config(opts...));
  }
  template <class T>
  inline Series full(pluss::msharp::RangeView::Int sizes, double fill_value, std::initializer_list<std::pair<const char*, T>> kv) {
    return full(sizes, fill_value, pluss::config(kv));
  }

  // Plain-`double` IEEE-754 constants -- the scalar counterparts of the 0-D-`Series`
  // factories below. Use these for bounds, sentinels, struct defaults, and scalar
  // comparisons (`NumericRule{.upper = pluss::series::scalar_inf}`,
  // `x.clip(-pluss::series::scalar_inf, cap)`). There is deliberately no `neg_inf`:
  // negate `scalar_inf` (or the Series `inf()`). A scalar has no device -- when it
  // enters a computation it is broadcast to wherever the tensors live.
  inline constexpr double scalar_inf = std::numeric_limits<double>::infinity();
  inline constexpr double scalar_nan = std::numeric_limits<double>::quiet_NaN();

  // Scalar constants (factories) -- these return a 0-D `Series` (Float32 by default,
  // CPU unless a Config with a platform is passed). For a bare `double`, use
  // `scalar_inf` / `scalar_nan` above instead.
  inline Series nan(ScalarType dt = ScalarType::Float32) { return codem::nan(dt); }
  inline Series nan(Config cfg) {
    return full({}, std::numeric_limits<double>::quiet_NaN(), cfg);
  }
  inline Series inf(ScalarType dt = ScalarType::Float32) { return codem::inf(dt); }
  inline Series inf(Config cfg) {
    return full({}, std::numeric_limits<double>::infinity(), cfg);
  }

  // Ranges / random
  inline Series arange(std::int64_t end, ScalarType dt = ScalarType::Int64) { return Series::arange(0, end, 1, dt); }
  inline Series arange(std::int64_t start, std::int64_t end, ScalarType dt = ScalarType::Int64) { return Series::arange(start, end, 1, dt); }
  inline Series arange(std::int64_t start, std::int64_t end, std::int64_t step, ScalarType dt = ScalarType::Int64) { return Series::arange(start, end, step, dt); }
  inline Series arange(std::int64_t start, std::int64_t end, Config cfg) {
    const ScalarType dt = cfg.has_dtype() ? cfg.dtype : ScalarType::Int64;
    const bool route_supported = pluss::cuda::supports_arange_factory_baseline(dt);
    if (FactoryCore::should_construct_native_cuda_factory(cfg, route_supported)) {
      return Series::arange(start, end, 1, dt, Platform::CUDA);
    }
    FactoryCore::check_cuda_factory_host_staging_allowed(cfg, "arange", route_supported);
    return FactoryCore::move_factory_to_config_platform_with_cuda_policy(
        Series::arange(start, end, 1, dt),
        cfg,
        "arange",
        route_supported);
  }
  inline Series arange(std::int64_t start, std::int64_t end, std::int64_t step, Config cfg) {
    const ScalarType dt = cfg.has_dtype() ? cfg.dtype : ScalarType::Int64;
    const bool route_supported = pluss::cuda::supports_arange_factory_baseline(dt);
    if (FactoryCore::should_construct_native_cuda_factory(cfg, route_supported)) {
      return Series::arange(start, end, step, dt, Platform::CUDA);
    }
    FactoryCore::check_cuda_factory_host_staging_allowed(cfg, "arange", route_supported);
    return FactoryCore::move_factory_to_config_platform_with_cuda_policy(
        Series::arange(start, end, step, dt),
        cfg,
        "arange",
        route_supported);
  }
  inline Series linspace(double start, double end, std::int64_t steps, ScalarType dt = ScalarType::Float32) { return Series::linspace(start, end, steps, dt); }
  inline Series linspace(double start, double end, std::int64_t steps, Config cfg) {
    const bool route_supported = pluss::cuda::supports_linspace_factory_baseline(cfg.dtype);
    if (FactoryCore::should_construct_native_cuda_factory(cfg, route_supported)) {
      return Series::linspace(start, end, steps, cfg.dtype, Platform::CUDA);
    }
    FactoryCore::check_cuda_factory_host_staging_allowed(cfg, "linspace", route_supported);
    return FactoryCore::move_factory_to_config_platform_with_cuda_policy(
        Series::linspace(start, end, steps, cfg.dtype),
        cfg,
        "linspace",
        route_supported);
  }
  inline Series eye(std::int64_t n, std::int64_t m = -1, ScalarType dt = ScalarType::Float32) { return Series::eye(n, m, dt); }
  inline Series eye(std::int64_t n, ScalarType dt) { return Series::eye(n, -1, dt); }
  inline Series eye(std::int64_t n, Config cfg) {
    const bool route_supported = pluss::cuda::supports_eye_factory_baseline(cfg.dtype);
    if (FactoryCore::should_construct_native_cuda_factory(cfg, route_supported)) {
      return Series::eye(n, -1, cfg.dtype, Platform::CUDA);
    }
    FactoryCore::check_cuda_factory_host_staging_allowed(cfg, "eye", route_supported);
    return FactoryCore::move_factory_to_config_platform_with_cuda_policy(
        Series::eye(n, -1, cfg.dtype),
        cfg,
        "eye",
        route_supported);
  }
  inline Series eye(std::int64_t n, std::int64_t m, Config cfg) {
    const bool route_supported = pluss::cuda::supports_eye_factory_baseline(cfg.dtype);
    if (FactoryCore::should_construct_native_cuda_factory(cfg, route_supported)) {
      return Series::eye(n, m, cfg.dtype, Platform::CUDA);
    }
    FactoryCore::check_cuda_factory_host_staging_allowed(cfg, "eye", route_supported);
    return FactoryCore::move_factory_to_config_platform_with_cuda_policy(
        Series::eye(n, m, cfg.dtype),
        cfg,
        "eye",
        route_supported);
  }

  inline Series rand(pluss::msharp::RangeView::Int sizes, ScalarType dt = ScalarType::Float32, std::uint64_t seed = 0) { return Series::rand(sizes, dt, seed); }
  inline Series rand(std::initializer_list<std::int64_t> sizes, ScalarType dt = ScalarType::Float32, std::uint64_t seed = 0) {return rand(pluss::msharp::RangeView::Int(sizes), dt, seed);}
  inline Series rand(pluss::msharp::RangeView::Int sizes, Config cfg, std::uint64_t seed = 0) {
    if (cfg.platform == Platform::MPS && cfg.dtype != ScalarType::Float64 &&
        pluss::codem::native::mps::supports_uniform_stochastic(ScalarType::Float32)) {
      return pluss::codem::native::mps::uniform_stochastic(sizes, ScalarType::Float32, 0.0, 1.0, seed).to(cfg.dtype);
    }
    const bool route_supported = pluss::cuda::supports_rand_factory_baseline(cfg.dtype);
    if (FactoryCore::should_construct_native_cuda_factory(cfg, route_supported)) {
      return Series::rand(sizes, cfg.dtype, seed, Platform::CUDA);
    }
    FactoryCore::check_cuda_factory_host_staging_allowed(cfg, "rand", route_supported);
    return FactoryCore::move_factory_to_config_platform_with_cuda_policy(
        Series::rand(sizes, cfg.dtype, seed),
        cfg,
        "rand",
        route_supported);
  }
  inline Series rand(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed = 0) {
    return rand(pluss::msharp::RangeView::Int(sizes), cfg, seed);
  }

  template <class... Opts>
  inline Series rand(pluss::msharp::RangeView::Int sizes, std::uint64_t seed, Opts... opts) {
    return rand(sizes, FactoryCore::make_config(opts...), seed);
  }
  template <class... Opts>
  inline Series rand(std::initializer_list<std::int64_t> sizes, std::uint64_t seed, Opts... opts) {
    const Config cfg = FactoryCore::make_config(opts...);
    return rand(pluss::msharp::RangeView::Int(sizes), cfg, seed);
  }

  inline Series rand_btw(f64_t lo,
                         f64_t hi,
                         pluss::msharp::RangeView::Int sizes,
                         std::uint64_t seed = 0) {
    PLUSS_CHECK(hi >= lo, "rand_btw: expected hi >= lo");
    return lo + (hi - lo) * rand(sizes, ScalarType::Float64, seed);
  }
  inline Series rand_btw(f64_t lo,
                         f64_t hi,
                         std::initializer_list<std::int64_t> sizes,
                         std::uint64_t seed = 0) {
    return rand_btw(lo, hi, pluss::msharp::RangeView::Int(sizes), seed);
  }
  inline Series rand_btw(f64_t lo,
                         f64_t hi,
                         pluss::msharp::RangeView::Int sizes,
                         Config cfg,
                         std::uint64_t seed = 0) {
    PLUSS_CHECK(hi >= lo, "rand_btw: expected hi >= lo");
    PLUSS_CHECK(cfg.dtype == ScalarType::Float32 || cfg.dtype == ScalarType::Float64,
                "rand_btw: CUDA native route supports float32/float64 outputs");
    return rand(sizes, cfg, seed).mul(hi - lo).add(lo);
  }
  inline Series rand_btw(f64_t lo,
                         f64_t hi,
                         std::initializer_list<std::int64_t> sizes,
                         Config cfg,
                         std::uint64_t seed = 0) {
    return rand_btw(lo, hi, pluss::msharp::RangeView::Int(sizes), cfg, seed);
  }
  inline Series rand_btw(f64_t lo, f64_t hi, i64_t size, std::uint64_t seed = 0) {
    return rand_btw(lo, hi, pluss::msharp::RangeView::Int(&size, 1), seed);
  }

  inline Series randn(pluss::msharp::RangeView::Int sizes, ScalarType dt = ScalarType::Float32, std::uint64_t seed = 0) { return Series::randn(sizes, dt, seed); }
  inline Series randn(std::initializer_list<std::int64_t> sizes, ScalarType dt = ScalarType::Float32, std::uint64_t seed = 0) {
    return randn(pluss::msharp::RangeView::Int(sizes), dt, seed);
  }
  inline Series randn(pluss::msharp::RangeView::Int sizes, Config cfg, std::uint64_t seed = 0) {
    if (cfg.platform == Platform::MPS && cfg.dtype != ScalarType::Float64 &&
        pluss::codem::native::mps::supports_normal_stochastic(ScalarType::Float32)) {
      return pluss::codem::native::mps::normal_stochastic(sizes, ScalarType::Float32, 0.0, 1.0, seed).to(cfg.dtype);
    }
    const bool route_supported = pluss::cuda::supports_randn_factory_baseline(cfg.dtype);
    if (FactoryCore::should_construct_native_cuda_factory(cfg, route_supported)) {
      return Series::randn(sizes, cfg.dtype, seed, Platform::CUDA);
    }
    FactoryCore::check_cuda_factory_host_staging_allowed(cfg, "randn", route_supported);
    return FactoryCore::move_factory_to_config_platform_with_cuda_policy(
        Series::randn(sizes, cfg.dtype, seed),
        cfg,
        "randn",
        route_supported);
  }
  inline Series randn(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed = 0) {
    return randn(pluss::msharp::RangeView::Int(sizes), cfg, seed);
  }
  template <class... Opts>
  inline Series randn(pluss::msharp::RangeView::Int sizes, std::uint64_t seed, Opts... opts) {
    return randn(sizes, FactoryCore::make_config(opts...), seed);
  }
  template <class... Opts>
  inline Series randn(std::initializer_list<std::int64_t> sizes, std::uint64_t seed, Opts... opts) {
    const Config cfg = FactoryCore::make_config(opts...);
    return randn(pluss::msharp::RangeView::Int(sizes), cfg, seed);
  }

  inline Series randint(std::int64_t lo, std::int64_t hi, pluss::msharp::RangeView::Int sizes, ScalarType dt = ScalarType::Int64, std::uint64_t seed = 0) {
    if (hi <= lo) pluss::codem::throw_error("randint: expected hi > lo");
    const double span = static_cast<double>(hi - lo);
    Series out = Series::rand(sizes, ScalarType::Float64, seed);
    out = out.mul(span).floor().add(static_cast<double>(lo));
    return out.to(dt);
  }
  inline Series randint(std::int64_t lo, std::int64_t hi, std::initializer_list<std::int64_t> sizes, ScalarType dt = ScalarType::Int64, std::uint64_t seed = 0) {
    return randint(lo, hi, pluss::msharp::RangeView::Int(sizes), dt, seed);
  }
  inline Series randint(std::int64_t lo, std::int64_t hi, pluss::msharp::RangeView::Int sizes, Config cfg, std::uint64_t seed = 0) {
    if (hi <= lo) pluss::codem::throw_error("randint: expected hi > lo");
    const ScalarType dt = cfg.has_dtype() ? cfg.dtype : ScalarType::Int64;
    if (cfg.platform == Platform::MPS &&
        pluss::codem::native::mps::supports_uniform_stochastic(ScalarType::Float32)) {
      const double span = static_cast<double>(hi - lo);
      Series out = pluss::codem::native::mps::uniform_stochastic(sizes, ScalarType::Float32, 0.0, 1.0, seed);
      out = out.mul(span).floor().add(static_cast<double>(lo));
      return out.to(dt);
    }
    const bool route_supported = pluss::cuda::supports_arange_factory_baseline(dt);
    if (FactoryCore::should_construct_native_cuda_factory(cfg.with_dtype(dt), route_supported)) {
      Series out = Series::empty(sizes, dt, Platform::CUDA);
      if (!pluss::codem::native::cuda::run_randint_factory(out, seed, lo, hi)) {
        pluss::codem::throw_error("randint: native CUDA randint kernel failed or dtype unsupported");
      }
      return out;
    }
    FactoryCore::check_cuda_factory_host_staging_allowed(cfg, "randint", route_supported);
    return FactoryCore::move_factory_to_config_platform_with_cuda_policy(
        randint(lo, hi, sizes, dt, seed),
        cfg,
        "randint",
        route_supported);
  }
  inline Series randint(std::int64_t lo, std::int64_t hi, std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed = 0) {
    return randint(lo, hi, pluss::msharp::RangeView::Int(sizes), cfg, seed);
  }
  template <class... Opts>
  inline Series randint(std::int64_t lo, std::int64_t hi, pluss::msharp::RangeView::Int sizes, std::uint64_t seed, Opts... opts) {
    return randint(lo, hi, sizes, FactoryCore::make_config(opts...), seed);
  }
  template <class... Opts>
  inline Series randint(std::int64_t lo, std::int64_t hi, std::initializer_list<std::int64_t> sizes, std::uint64_t seed, Opts... opts) {
    return randint(lo, hi, pluss::msharp::RangeView::Int(sizes), FactoryCore::make_config(opts...), seed);
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
  inline Series logspace(double start,
                         double end,
                         std::int64_t steps,
                         double base,
                         Config cfg) {
    PLUSS_CHECK(steps > 0, "logspace: expected steps > 0");
    PLUSS_CHECK(base > 0.0 && base != 1.0, "logspace: expected base > 0 and base != 1");
    PLUSS_CHECK(cfg.dtype == ScalarType::Float32 || cfg.dtype == ScalarType::Float64,
                "logspace: CUDA native route supports float32/float64 outputs");
    auto exponents = linspace(start, end, steps, cfg);
    return exponents.mul(std::log(base)).exp();
  }
  inline Series logspace(double start,
                         double end,
                         std::int64_t steps,
                         Config cfg) {
    return logspace(start, end, steps, 10.0, cfg);
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
    const bool route_supported = pluss::cuda::supports_random_stochastic_baseline(dt);
    if (cfg.platform == Platform::CUDA &&
        pluss::cuda::native_stochastic_enabled() &&
        route_supported) {
      Series out = Series::empty({n}, dt, Platform::CUDA);
      if (!pluss::codem::native::cuda::run_randperm_factory(out, seed)) {
        pluss::codem::throw_error("randperm: native CUDA randperm kernel failed or dtype unsupported");
      }
      return out;
    }
    FactoryCore::check_cuda_stochastic_host_staging_allowed(cfg, "randperm", route_supported);
    return FactoryCore::move_stochastic_factory_to_config_platform_with_cuda_policy(
        randperm(n, dt, seed),
        cfg,
        "randperm",
        route_supported);
  }

  inline Series normal(double mean,
                       double std,
                       pluss::msharp::RangeView::Int sizes,
                       ScalarType dt = ScalarType::Float32,
                       std::uint64_t seed = 0) {
    PLUSS_CHECK(std >= 0.0, "normal: expected std >= 0");
    return Series::randn(sizes, dt, seed).mul(std).add(mean);
  }
  inline Series normal(double mean,
                       double std,
                       pluss::msharp::RangeView::Int sizes,
                       Config cfg,
                       std::uint64_t seed = 0) {
    PLUSS_CHECK(std >= 0.0, "normal: expected std >= 0");
    if (cfg.platform == Platform::MPS && cfg.dtype != ScalarType::Float64 &&
        pluss::codem::native::mps::supports_normal_stochastic(ScalarType::Float32)) {
      return pluss::codem::native::mps::normal_stochastic(sizes, ScalarType::Float32, mean, std, seed).to(cfg.dtype);
    }
    const bool route_supported = pluss::cuda::supports_normal_stochastic_baseline(cfg.dtype);
    if (cfg.platform == Platform::CUDA &&
        pluss::cuda::native_stochastic_enabled() &&
        route_supported) {
      Series out = Series::empty(sizes, cfg.dtype, Platform::CUDA);
      if (!pluss::codem::native::cuda::run_normal_factory(out, seed, mean, std)) {
        pluss::codem::throw_error("normal: native CUDA normal kernel failed or dtype unsupported");
      }
      return out;
    }
    FactoryCore::check_cuda_stochastic_host_staging_allowed(cfg, "normal", route_supported);
    return FactoryCore::move_stochastic_factory_to_config_platform_with_cuda_policy(
        Series::randn(sizes, cfg.dtype, seed).mul(std).add(mean),
        cfg,
        "normal",
        route_supported);
  }
  inline Series normal(double mean,
                       double std,
                       std::initializer_list<std::int64_t> sizes,
                       ScalarType dt = ScalarType::Float32,
                       std::uint64_t seed = 0) {
    return normal(mean, std, pluss::msharp::RangeView::Int(sizes), dt, seed);
  }
  inline Series normal(double mean,
                       double std,
                       std::initializer_list<std::int64_t> sizes,
                       Config cfg,
                       std::uint64_t seed = 0) {
    return normal(mean, std, pluss::msharp::RangeView::Int(sizes), cfg, seed);
  }

  inline Series bernoulli(pluss::msharp::RangeView::Int sizes,
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
  inline Series bernoulli(pluss::msharp::RangeView::Int sizes,
                          double p,
                          Config cfg,
                          std::uint64_t seed = 0) {
    ScalarType dt = cfg.dtype;
    if (dt == ScalarType::Float32) dt = ScalarType::Bool;
    if (cfg.platform == Platform::MPS &&
        pluss::codem::native::mps::supports_uniform_stochastic(ScalarType::Float32)) {
      PLUSS_CHECK(p >= 0.0 && p <= 1.0, "bernoulli: expected 0 <= p <= 1");
      Series out = pluss::codem::native::mps::uniform_stochastic(sizes, ScalarType::Float32, 0.0, 1.0, seed);
      out = out.lt(p);
      return (dt == ScalarType::Bool) ? out : out.to(dt);
    }
    const bool route_supported = pluss::cuda::supports_bernoulli_stochastic_baseline(dt);
    if (cfg.platform == Platform::CUDA &&
        pluss::cuda::native_stochastic_enabled() &&
        route_supported) {
      Series out = Series::empty(sizes, dt, Platform::CUDA);
      if (!pluss::codem::native::cuda::run_bernoulli_factory(out, seed, p)) {
        pluss::codem::throw_error("bernoulli: native CUDA bernoulli kernel failed or dtype unsupported");
      }
      return out;
    }
    FactoryCore::check_cuda_stochastic_host_staging_allowed(cfg.with_dtype(dt), "bernoulli", route_supported);
    return FactoryCore::move_stochastic_factory_to_config_platform_with_cuda_policy(
        bernoulli(sizes, p, dt, seed),
        cfg,
        "bernoulli",
        route_supported);
  }
  inline Series bernoulli(std::initializer_list<std::int64_t> sizes,
                          double p = 0.5,
                          ScalarType dt = ScalarType::Bool,
                          std::uint64_t seed = 0) {
    return bernoulli(pluss::msharp::RangeView::Int(sizes), p, dt, seed);
  }
  inline Series bernoulli(std::initializer_list<std::int64_t> sizes,
                          double p,
                          Config cfg,
                          std::uint64_t seed = 0) {
    return bernoulli(pluss::msharp::RangeView::Int(sizes), p, cfg, seed);
  }

  inline Series poisson(pluss::msharp::RangeView::Int sizes,
                        double lambda,
                        ScalarType dt = ScalarType::Int64,
                        std::uint64_t seed = 0) {
    PLUSS_CHECK(lambda >= 0.0 && std::isfinite(lambda), "poisson: expected finite lambda >= 0");
    std::int64_t n = 1;
    for (const auto s : sizes) {
      PLUSS_CHECK(s >= 0, "poisson: sizes must be >= 0");
      n *= s;
    }
    std::vector<std::int64_t> samples(static_cast<std::size_t>(n), 0);
    auto rng = FactoryCore::make_rng(seed);
    std::poisson_distribution<std::int64_t> dist(lambda);
    for (auto& sample : samples) {
      sample = dist(rng);
    }
    Series out = codem::make_series<std::int64_t>(samples, sizes, Config{}.with_dtype(ScalarType::Int64));
    return (dt == ScalarType::Int64) ? out : out.to(dt);
  }
  inline Series poisson(std::initializer_list<std::int64_t> sizes,
                        double lambda,
                        ScalarType dt = ScalarType::Int64,
                        std::uint64_t seed = 0) {
    return poisson(pluss::msharp::RangeView::Int(sizes), lambda, dt, seed);
  }
  inline Series poisson(pluss::msharp::RangeView::Int sizes,
                        double lambda,
                        Config cfg,
                        std::uint64_t seed = 0) {
    ScalarType dt = cfg.dtype;
    if (dt == ScalarType::Float32) dt = ScalarType::Int64;
    const bool route_supported = false;
    FactoryCore::check_cuda_stochastic_host_staging_allowed(cfg.with_dtype(dt), "poisson", route_supported);
    return FactoryCore::move_stochastic_factory_to_config_platform_with_cuda_policy(
        poisson(sizes, lambda, dt, seed),
        cfg.with_dtype(dt),
        "poisson",
        route_supported);
  }
  inline Series poisson(std::initializer_list<std::int64_t> sizes,
                        double lambda,
                        Config cfg,
                        std::uint64_t seed = 0) {
    return poisson(pluss::msharp::RangeView::Int(sizes), lambda, cfg, seed);
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
        false &&
        !probs.empty() &&
        num_samples >= 0 &&
        (replacement || num_samples <= static_cast<std::int64_t>(probs.size()));
    FactoryCore::check_cuda_stochastic_host_staging_allowed(cfg, "multinomial", route_supported);
    return FactoryCore::move_stochastic_factory_to_config_platform_with_cuda_policy(
        multinomial(probs, num_samples, replacement, seed),
        cfg,
        "multinomial",
        route_supported);
  }

  // Like-factories
  inline Series empty_like(const Series& like, ScalarType dt) {
    return FactoryCore::preserve_like_layout(
        empty(like.sizes(), Config{}.with_dtype(dt).with_platform(like.platform())), like);
  }
  inline Series empty_like(const Series& like, Config cfg = {}) {
    return FactoryCore::preserve_like_layout(
        empty(like.sizes(), FactoryCore::resolve_like_config(like, cfg)), like);
  }
  template <class... Opts>
  inline Series empty_like(const Series& like, Opts... opts) {
    return empty_like(like, FactoryCore::make_config(opts...));
  }

  inline Series zeros_like(const Series& like, ScalarType dt) {
    return FactoryCore::preserve_like_layout(
        zeros(like.sizes(), Config{}.with_dtype(dt).with_platform(like.platform())), like);
  }
  inline Series zeros_like(const Series& like, Config cfg = {}) {
    return FactoryCore::preserve_like_layout(
        zeros(like.sizes(), FactoryCore::resolve_like_config(like, cfg)), like);
  }
  template <class... Opts>
  inline Series zeros_like(const Series& like, Opts... opts) {
    return zeros_like(like, FactoryCore::make_config(opts...));
  }
  inline Series ones_like(const Series& like, ScalarType dt) {
    return FactoryCore::preserve_like_layout(
        ones(like.sizes(), Config{}.with_dtype(dt).with_platform(like.platform())), like);
  }
  inline Series ones_like(const Series& like, Config cfg = {}) {
    return FactoryCore::preserve_like_layout(
        ones(like.sizes(), FactoryCore::resolve_like_config(like, cfg)), like);
  }
  template <class... Opts>
  inline Series ones_like(const Series& like, Opts... opts) {
    return ones_like(like, FactoryCore::make_config(opts...));
  }

  inline Series full_like(const Series& like, double fill_value, ScalarType dt) {
    return FactoryCore::preserve_like_layout(
        full(like.sizes(), fill_value, Config{}.with_dtype(dt).with_platform(like.platform())), like);
  }
  inline Series full_like(const Series& like, double fill_value, Config cfg = {}) {
    return FactoryCore::preserve_like_layout(
        full(like.sizes(), fill_value, FactoryCore::resolve_like_config(like, cfg)), like);
  }
  template <class... Opts>
  inline Series full_like(const Series& like, double fill_value, Opts... opts) {
    return full_like(like, fill_value, FactoryCore::make_config(opts...));
  }

  inline Series rand_like(const Series& like, ScalarType dt, std::uint64_t seed = 0) {
    return FactoryCore::preserve_like_layout(
        rand(like.sizes(), Config{}.with_dtype(dt).with_platform(like.platform()), seed), like);
  }
  inline Series rand_like(const Series& like, Config cfg = {}, std::uint64_t seed = 0) {
    return FactoryCore::preserve_like_layout(
        rand(like.sizes(), FactoryCore::resolve_like_config(like, cfg), seed), like);
  }

  inline Series randn_like(const Series& like, ScalarType dt, std::uint64_t seed = 0) {
    return FactoryCore::preserve_like_layout(
        randn(like.sizes(), Config{}.with_dtype(dt).with_platform(like.platform()), seed), like);
  }
  inline Series randn_like(const Series& like, Config cfg = {}, std::uint64_t seed = 0) {
    return FactoryCore::preserve_like_layout(
        randn(like.sizes(), FactoryCore::resolve_like_config(like, cfg), seed), like);
  }

  inline Series randint_like(const Series& like,
                             std::int64_t hi,
                             ScalarType dt,
                             std::uint64_t seed = 0) {
    return FactoryCore::preserve_like_layout(randint(
        /*lo=*/0,
        hi,
        like.sizes(),
        Config{}.with_dtype(dt).with_platform(like.platform()),
        seed), like);
  }
  inline Series randint_like(const Series& like,
                             std::int64_t lo,
                             std::int64_t hi,
                             ScalarType dt,
                             std::uint64_t seed = 0) {
    return FactoryCore::preserve_like_layout(randint(
        lo,
        hi,
        like.sizes(),
        Config{}.with_dtype(dt).with_platform(like.platform()),
        seed), like);
  }
  inline Series randint_like(const Series& like,
                             std::int64_t hi,
                             Config cfg = {},
                             std::uint64_t seed = 0) {
    return FactoryCore::preserve_like_layout(randint(
        /*lo=*/0,
        hi,
        like.sizes(),
        FactoryCore::resolve_like_config(like, cfg),
        seed), like);
  }
  inline Series randint_like(const Series& like,
                             std::int64_t lo,
                             std::int64_t hi,
                             Config cfg = {},
                             std::uint64_t seed = 0) {
    return FactoryCore::preserve_like_layout(randint(
        lo,
        hi,
        like.sizes(),
        FactoryCore::resolve_like_config(like, cfg),
        seed), like);
  }

  inline Series eye_like(const Series& like, ScalarType dt) {
    PLUSS_CHECK(like.dim() >= 2, "eye_like: expected input with rank >= 2");
    const auto n = like.sizes()[static_cast<std::size_t>(like.dim() - 2)];
    const auto m = like.sizes()[static_cast<std::size_t>(like.dim() - 1)];
    return eye(n, m, Config{}.with_dtype(dt).with_platform(like.platform()));
  }
  inline Series eye_like(const Series& like, Config cfg = {}) {
    PLUSS_CHECK(like.dim() >= 2, "eye_like: expected input with rank >= 2");
    const auto n = like.sizes()[static_cast<std::size_t>(like.dim() - 2)];
    const auto m = like.sizes()[static_cast<std::size_t>(like.dim() - 1)];
    return eye(n, m, FactoryCore::resolve_like_config(like, cfg));
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
    PLUSS_CHECK(real.platform() == imag.platform(),
                "complex: real and imag must be on the same platform");

    const ScalarType base_dt = (dt == ScalarType::Complex128) ? ScalarType::Float64 : ScalarType::Float32;
    if (real.platform() == Platform::CUDA) {
      const bool route_supported =
          pluss::cuda::native_factory_enabled() &&
          real.scalar_type() == base_dt &&
          imag.scalar_type() == base_dt;
      FactoryCore::check_cuda_factory_host_staging_allowed(
          Config{}.with_dtype(dt).with_platform(Platform::CUDA),
          "complex",
          route_supported);
      if (route_supported) {
        auto rr = real.contiguous();
        auto ii = imag.contiguous();
        auto out = Series::empty(real.sizes(), dt, Platform::CUDA);
        if (!pluss::codem::native::cuda::run_complex_factory(rr, ii, out)) {
          pluss::codem::throw_error("complex: native CUDA complex constructor failed");
        }
        return out;
      }
    }
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
    PLUSS_CHECK(abs.platform() == angle.platform(),
                "polar: abs and angle must be on the same platform");
    const ScalarType base_dt = (dt == ScalarType::Complex128) ? ScalarType::Float64 : ScalarType::Float32;
    if (abs.platform() == Platform::CUDA) {
      const bool route_supported =
          pluss::cuda::native_factory_enabled() &&
          abs.scalar_type() == base_dt &&
          angle.scalar_type() == base_dt;
      FactoryCore::check_cuda_factory_host_staging_allowed(
          Config{}.with_dtype(dt).with_platform(Platform::CUDA),
          "polar",
          route_supported);
      if (route_supported) {
        auto mag = abs.contiguous();
        auto ang = angle.contiguous();
        auto out = Series::empty(abs.sizes(), dt, Platform::CUDA);
        if (!pluss::codem::native::cuda::run_polar_factory(mag, ang, out)) {
          pluss::codem::throw_error("polar: native CUDA polar constructor failed");
        }
        return out;
      }
    }
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
    return pluss::series::undefined();
  }
  template <class... Args>
  inline Series sparse_csr_tensor(Args&&...) {
    pluss::codem::throw_error("sparse_csr_tensor: not implemented yet for CPU");
    return pluss::series::undefined();
  }
  template <class... Args>
  inline Series sparse_csc_tensor(Args&&...) {
    pluss::codem::throw_error("sparse_csc_tensor: not implemented yet for CPU");
    return pluss::series::undefined();
  }
  template <class... Args>
  inline Series sparse_bsr_tensor(Args&&...) {
    pluss::codem::throw_error("sparse_bsr_tensor: not implemented yet for CPU");
    return pluss::series::undefined();
  }
  template <class... Args>
  inline Series sparse_bsc_tensor(Args&&...) {
    pluss::codem::throw_error("sparse_bsc_tensor: not implemented yet for CPU");
    return pluss::series::undefined();
  }
  template <class... Args>
  inline Series quantize_per_tensor(Args&&...) {
    pluss::codem::throw_error("quantize_per_tensor: not implemented yet for CPU");
    return pluss::series::undefined();
  }
  template <class... Args>
  inline Series quantize_per_channel(Args&&...) {
    pluss::codem::throw_error("quantize_per_channel: not implemented yet for CPU");
    return pluss::series::undefined();
  }

// interop with other libraries (e.g., Torch)
#ifdef PLUSS_ENABLE_TORCH
  // Construct a pluss::Series that views a CPU torch::Tensor (zero-copy; the
  // tensor is kept alive by the Series). The reverse direction is the method
  // Series::tensor() -- there is deliberately no free to_torch function.
  inline Series from_torch(const ::torch::Tensor& t) { return codem::from_torch(t); }
#endif

  // --------------------------------------------------------------------------
  // Value / blob / make factories (create NEW Series from user data)
  // --------------------------------------------------------------------------
  // pluss::series::print(s) -- human-readable rendering, mirroring
  // pluss::note::print/pluss::timeline::print. `std::cout << s` also works
  // directly via pluss::codem's Series operator<<.
  inline std::string print(const pluss::Series& s) {
    return pluss::codem::print(s);
  }
  inline std::string print(const pluss::Series& s, const pluss::PrintOptions& opt) {
    return pluss::codem::print(s, opt);
  }
  inline std::ostream& print(std::ostream& os, const pluss::Series& s,
                             const pluss::PrintOptions& opt = {}) {
    return pluss::codem::print(os, s, opt);
  }

  inline pluss::Series select(pluss::msharp::RangeView::Of<pluss::Series> conditions,
                              pluss::msharp::RangeView::Of<pluss::Series> choices,
                              const pluss::Series& default_value) {
    return pluss::detail::select_core::fold_select(
        "pluss::series::select",
        conditions,
        choices,
        default_value,
        [](const pluss::Series& cond, const pluss::Series& choice, const pluss::Series& out) {
          if (choice.scalar_type() == pluss::ScalarType::Bool
              && out.scalar_type() == pluss::ScalarType::Bool) {
            return cond.logical_and(choice).logical_or(cond.logical_not().logical_and(out));
          }
          return cond.where(choice, out);
        });
  }

  inline pluss::Series select(std::initializer_list<pluss::Series> conditions,
                              std::initializer_list<pluss::Series> choices,
                              const pluss::Series& default_value) {
    return pluss::detail::select_core::select_init(
        "pluss::series::select",
        conditions,
        choices,
        default_value,
        [](std::string_view,
           pluss::msharp::RangeView::Of<pluss::Series> conds,
           pluss::msharp::RangeView::Of<pluss::Series> picks,
           const pluss::Series& fallback) {
          return select(conds, picks, fallback);
        });
  }

  inline pluss::Series select(const pluss::Series& conditions,
                              const pluss::Series& choices,
                              const pluss::Series& default_value) {
    return pluss::detail::select_core::select_scalar(
        "pluss::series::select",
        conditions,
        choices,
        default_value,
        [](std::string_view,
           pluss::msharp::RangeView::Of<pluss::Series> conds,
           pluss::msharp::RangeView::Of<pluss::Series> picks,
           const pluss::Series& fallback) {
          return select(conds, picks, fallback);
        });
  }

  template <pluss::codem::SeriesScalar T>
  inline pluss::Series select(pluss::msharp::RangeView::Of<pluss::Series> conditions,
                              pluss::msharp::RangeView::Of<pluss::Series> choices,
                              T default_value) {
    PLUSS_CHECK(!choices.empty(), "pluss::series::select: at least one choice is required");
    return select(
        conditions,
        choices,
        pluss::codem::make_scalar_like(choices[0], static_cast<double>(default_value)));
  }

  inline pluss::Series select(pluss::msharp::RangeView::Of<pluss::Series> conditions,
                              pluss::msharp::RangeView::Of<pluss::Series> choices,
                              bool default_value) {
    PLUSS_CHECK(!choices.empty(), "pluss::series::select: at least one choice is required");
    return select(
        conditions,
        choices,
        pluss::Series::full(
            {},
            default_value ? 1.0 : 0.0,
            pluss::ScalarType::Bool,
            choices[0].platform()));
  }

  template <pluss::codem::SeriesScalar T>
  inline pluss::Series select(std::initializer_list<pluss::Series> conditions,
                              std::initializer_list<pluss::Series> choices,
                              T default_value) {
    return select(
        pluss::msharp::RangeView::Of<pluss::Series>(conditions),
        pluss::msharp::RangeView::Of<pluss::Series>(choices),
        default_value);
  }

  template <pluss::codem::SeriesScalar T>
  inline pluss::Series select(pluss::Series conditions,
                              pluss::Series choices,
                              T default_value) {
    return select(
        pluss::msharp::RangeView::Of<pluss::Series>({conditions}),
        pluss::msharp::RangeView::Of<pluss::Series>({choices}),
        default_value);
  }

  inline pluss::Series select(std::initializer_list<pluss::Series> conditions,
                              std::initializer_list<pluss::Series> choices,
                              bool default_value) {
    return select(
        pluss::msharp::RangeView::Of<pluss::Series>(conditions),
        pluss::msharp::RangeView::Of<pluss::Series>(choices),
        default_value);
  }

  // Owned-construction entrypoint.
  // Data forms are parsed by DataReader constructors; options are applied here.
  template <class... Opts>
  inline Series make(detail::DataReader data,
                     detail::DataReader::Option first = {},
                     Opts&&... opts) {
    return codem::value(
        data,
        std::move(first),
        std::forward<Opts>(opts)...);
  }

  inline Series from_host(void* data,
                             pluss::msharp::RangeView::Int sizes,
                             std::span<const std::int64_t> strides,
                             BlobDeleter deleter,
                             ScalarType dtype = ScalarType::Float32) {
    return codem::from_buffer(data, sizes, strides, deleter, dtype);
  }

  // Fully qualified so this resolves to the StackSpan-taking overload in
  // stack_array.h (which stays in the outer `pluss::` namespace as a shared
  // container utility, not part of this factory surface) rather than
  // re-finding this very overload -- ordinary unqualified lookup would stop
  // at `pluss::series::pile` before ever reaching the enclosing `pluss::`.
  inline pluss::Series pile(std::initializer_list<Series> values, std::int64_t dim = 0) {
    return ::pluss::pile(pluss::msharp::Lister::Series(values), dim);
  }

  inline pluss::Series pile(pluss::msharp::Lister::Series values, std::int64_t dim = 0) {
    return ::pluss::pile(values, dim);
  }

  inline pluss::Series unite(std::initializer_list<Series> values,
                             std::int64_t dim = 0,
                             bool strict_dtype = true) {
    return ::pluss::unite(pluss::msharp::Lister::Series(values), dim, strict_dtype);
  }

  inline pluss::Series unite(pluss::msharp::Lister::Series values,
                             std::int64_t dim = 0,
                             bool strict_dtype = true) {
    return ::pluss::unite(values, dim, strict_dtype);
  }

} // namespace pluss::series

// Keep the backward-compatible flat pluss::* spellings transitively
// available from series_factory.h. Kept in a separate file since Series'
// flat names (used across the entire codebase, not slated for a hard-rename
// sweep) would make one file unwieldy -- mirrors the note_factory.h /
// note_factory_compat.h split.
#include "pluss/codem/series_factory_compat.h"
