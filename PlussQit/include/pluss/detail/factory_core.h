#pragma once

#include "pluss/config.h"
#include "pluss/codem/codem.h"
#include "aux/rng_seed.h"

#include <algorithm>
#include <cstdint>
#include <numeric>
#include <random>
#include <span>
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

  static std::mt19937_64 make_rng(std::uint64_t seed) {
    return std::mt19937_64(pluss::aux::seed_or_next(seed));
  }

  static bool same_shape(std::span<const std::int64_t> a, std::span<const std::int64_t> b) {
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
  }
};

} // namespace pluss::detail
