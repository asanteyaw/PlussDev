#pragma once

#include "pluss/config.h"
#include "pluss/codem/codem.h"
#include "pluss/tils/auxiliary/rng_seed.h"

#include <algorithm>
#include <cstdint>
#include <numeric>
#include <random>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

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
    return cfg.has_dtype() ? cfg.dtype : like.scalar_type();
  }

  static Platform resolve_like_platform(const Series& like, Config cfg) {
    return cfg.has_platform() ? cfg.platform : like.platform();
  }

  static Config resolve_like_config(const Series& like, Config cfg) {
    return cfg
        .with_dtype(resolve_like_dtype(like, cfg))
        .with_platform(resolve_like_platform(like, cfg));
  }

  // Mirrors ATen's infer_dense_strides: retain the source's dimension order
  // while compacting sliced or overlapping layouts into safe dense storage.
  static std::vector<std::int64_t> infer_like_strides(const Series& like) {
    const auto sizes = like.sizes();
    const auto strides = like.strides();
    const std::size_t ndim = sizes.size();
    if (ndim == 0) return {};

    bool empty = false;
    std::vector<std::size_t> dense_order;
    dense_order.reserve(ndim);
    for (std::size_t dim = 0; dim < ndim; ++dim) {
      empty = empty || sizes[dim] == 0;
      if (sizes[dim] > 1) dense_order.push_back(dim);
    }
    std::stable_sort(dense_order.begin(), dense_order.end(), [&](std::size_t lhs, std::size_t rhs) {
      return strides[lhs] < strides[rhs];
    });
    bool non_overlapping_dense = true;
    std::int64_t expected = 1;
    for (const auto dim : dense_order) {
      if (strides[dim] != expected) {
        non_overlapping_dense = false;
        break;
      }
      expected *= sizes[dim];
    }
    if (empty || non_overlapping_dense) {
      return std::vector<std::int64_t>(strides.begin(), strides.end());
    }
    if (ndim == 1) return {1};

    std::vector<std::size_t> perm(ndim);
    std::iota(perm.rbegin(), perm.rend(), 0);
    const auto compare = [&](std::size_t dim0, std::size_t dim1) {
      const auto stride0 = strides[dim0];
      const auto stride1 = strides[dim1];
      if (stride0 == 0 || stride1 == 0) return 0;
      if (stride0 < stride1) return -1;
      if (stride0 > stride1) return 1;
      return sizes[dim0] > sizes[dim1] ? 1 : 0;
    };

    for (std::size_t i = 1; i < ndim; ++i) {
      std::size_t dim1 = i;
      for (std::size_t j = 1; j <= i; ++j) {
        const std::size_t dim0 = i - j;
        const int ordering = compare(perm[dim0], perm[dim1]);
        if (ordering > 0) {
          std::swap(perm[dim0], perm[dim1]);
          dim1 = dim0;
        } else if (ordering < 0) {
          break;
        }
      }
    }

    std::vector<std::int64_t> out(ndim);
    std::int64_t running = 1;
    for (const auto dim : perm) {
      out[dim] = running;
      if (sizes[dim] > 1) running *= sizes[dim];
    }
    return out;
  }

  static Series preserve_like_layout(Series result, const Series& like) {
    const auto strides = infer_like_strides(like);
    if (std::equal(strides.begin(), strides.end(), result.strides().begin(), result.strides().end())) {
      return result;
    }
    // MPS caches device buffers per SeriesImpl. Materialize a native factory
    // result to its shared host storage before replacing its metadata view so
    // the new impl cannot observe an uninitialized host mirror.
    if (result.platform() == Platform::MPS) {
      (void)static_cast<const Series&>(result).raw_data();
    }
    Series view = result.as_strided(like.sizes(), pluss::msharp::RangeView::Int(strides));
    view.detach_();
    return view;
  }

  static Platform resolve_config_platform(Config cfg) {
    return cfg.platform;
  }

  static bool should_construct_native_cuda_factory(Config cfg, bool route_supported) {
    return resolve_config_platform(cfg) == Platform::CUDA &&
        pluss::cuda::native_factory_enabled() &&
        route_supported;
  }

  static void check_cuda_factory_host_staging_allowed(
      Config cfg,
      std::string_view op_name,
      bool route_supported) {
    const Platform target = resolve_config_platform(cfg);
    if (target != Platform::CUDA) {
      return;
    }

    const bool native_enabled = pluss::cuda::native_factory_enabled();
    const bool native_strict = pluss::cuda::native_factory_strict();

    if (native_enabled && route_supported) {
      return;
    }
    if (native_strict) {
      pluss::codem::throw_error(
          std::string(op_name) +
          ": strict native CUDA factory route requested but kernel is unsupported");
    }
    if (!pluss::cuda::cpu_bridge_enabled()) {
      pluss::codem::throw_error(
          std::string(op_name) +
          ": CUDA factory path unavailable in this tranche; no CPU fallback allowed. "
          "Move Series to CPU explicitly or enable PLUSS_CUDA_ALLOW_CPU_BRIDGE=1.");
    }
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

  static void check_cuda_stochastic_host_staging_allowed(
      Config cfg,
      std::string_view op_name,
      bool route_supported) {
    const Platform target = resolve_config_platform(cfg);
    if (target != Platform::CUDA) {
      return;
    }

    const bool native_enabled = pluss::cuda::native_stochastic_enabled();
    const bool native_strict = pluss::cuda::native_stochastic_strict();

    if (native_enabled && route_supported) {
      return;
    }
    if (native_strict) {
      pluss::codem::throw_error(
          std::string(op_name) +
          ": strict native CUDA stochastic route requested but kernel is unsupported");
    }
    if (!pluss::cuda::cpu_bridge_enabled()) {
      pluss::codem::throw_error(
          std::string(op_name) +
          ": CUDA stochastic path unavailable in this tranche; no CPU fallback allowed. "
          "Move Series to CPU explicitly or enable PLUSS_CUDA_ALLOW_CPU_BRIDGE=1.");
    }
  }

  static std::mt19937_64 make_rng(std::uint64_t seed) {
    return std::mt19937_64(pluss::tils::seed_or_next(seed));
  }

  static bool same_shape(std::span<const std::int64_t> a, std::span<const std::int64_t> b) {
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
  }
};

} // namespace pluss::detail
