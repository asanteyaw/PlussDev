#pragma once

#include "pluss/config.h"
#include "pluss/codem/codem.h"
#include "pluss/tils/aux/rng_seed.h"

#include <algorithm>
#include <cstdint>
#include <numeric>
#include <random>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace pluss::detail {

template <class T>
inline constexpr bool always_false_factory_v = false;

struct FactoryCore final {
  static void apply_opt(Config& c, cfg::DType o) { c = c.with_dtype(o.v); }
  static void apply_opt(Config& c, cfg::Pinned o) { c = c.with_pinned(o.v); }
  static void apply_opt(Config& c, cfg::Platform o) { c = c.with_platform(o.v); }
  static void apply_opt(Config& c, const Config& o) { c = o; }
  static void apply_opt(Config& c, Config&& o) { c = std::move(o); }
  template <class T>
  static void apply_opt(Config&, const T&) {
    static_assert(always_false_factory_v<std::remove_cvref_t<T>>, "Unknown Config option type");
  }

  template <class... Opts>
  static Config make_config(Opts... opts) {
    Config c{};
    (apply_opt(c, opts), ...);
    return c;
  }

  static ScalarType resolve_like_dtype(const Series& like, Config cfg) {
    // Keep Torch-like policy for *_like: inherit dtype unless user overrides.
    return (cfg.dtype == ScalarType::Float32) ? like.scalar_type() : cfg.dtype;
  }

  static Platform resolve_config_platform(Config cfg) {
    return cfg.platform;
  }

  static Series move_to_config_platform(Series s, Config cfg) {
    const Platform target = resolve_config_platform(cfg);
    return (s.platform() == target) ? s : s.to(target);
  }

  static Series move_factory_to_config_platform_with_cuda_policy(
      Series s,
      Config cfg,
      std::string_view op_name,
      bool route_supported) {
    const Platform target = resolve_config_platform(cfg);
    if (target != Platform::CUDA) {
      return (s.platform() == target) ? s : s.to(target);
    }

    const bool native_enabled = pluss::cuda::native_factory_enabled();
    const bool native_strict = pluss::cuda::native_factory_strict();

    if (native_enabled && route_supported) {
      if (!pluss::cuda::cpu_bridge_enabled()) {
        pluss::codem::throw_error(
            std::string(op_name) +
            ": CUDA native factory route declared but kernel is not implemented yet");
      }
      return (s.platform() == target) ? s : s.to(target);
    }
    if (native_strict) {
      pluss::codem::throw_error(
          std::string(op_name) +
          ": strict native CUDA factory route requested but kernel is unsupported");
    }
    if (pluss::cuda::cpu_bridge_enabled()) {
      return (s.platform() == target) ? s : s.to(target);
    }
    pluss::codem::throw_error(
        std::string(op_name) +
        ": CUDA factory path unavailable in this tranche; no CPU fallback allowed. "
        "Move Series to CPU explicitly or enable PLUSS_CUDA_ALLOW_CPU_BRIDGE=1.");
    return s;
  }

  static Series move_stochastic_factory_to_config_platform_with_cuda_policy(
      Series s,
      Config cfg,
      std::string_view op_name,
      bool route_supported) {
    const Platform target = resolve_config_platform(cfg);
    if (target != Platform::CUDA) {
      return (s.platform() == target) ? s : s.to(target);
    }

    const bool native_enabled = pluss::cuda::native_stochastic_enabled();
    const bool native_strict = pluss::cuda::native_stochastic_strict();

    if (native_enabled && route_supported) {
      if (!pluss::cuda::cpu_bridge_enabled()) {
        pluss::codem::throw_error(
            std::string(op_name) +
            ": CUDA native stochastic route declared but kernel is not implemented yet");
      }
      return (s.platform() == target) ? s : s.to(target);
    }
    if (native_strict) {
      pluss::codem::throw_error(
          std::string(op_name) +
          ": strict native CUDA stochastic route requested but kernel is unsupported");
    }
    if (pluss::cuda::cpu_bridge_enabled()) {
      return (s.platform() == target) ? s : s.to(target);
    }
    pluss::codem::throw_error(
        std::string(op_name) +
        ": CUDA stochastic path unavailable in this tranche; no CPU fallback allowed. "
        "Move Series to CPU explicitly or enable PLUSS_CUDA_ALLOW_CPU_BRIDGE=1.");
    return s;
  }

  static std::mt19937_64 make_rng(std::uint64_t seed) {
    return std::mt19937_64(pluss::aux::seed_or_next(seed));
  }

  static bool same_shape(std::span<const std::int64_t> a, std::span<const std::int64_t> b) {
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
  }
};

} // namespace pluss::detail
