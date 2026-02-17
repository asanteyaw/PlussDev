#pragma once

#include "pluss/export.h"
#include "pluss/config.h"
#include "pluss/codem/codem.h"
#include "CodeM/core/Config.h"

#include <cstdint>
#include <initializer_list>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

#include "pluss/detail/factory_deduction.h"

#ifdef PLUSS_ENABLE_TORCH
#include <torch/torch.h>
#include <pluss/codem/torch_interop.h>
#endif

namespace pluss {
  using Series = codem::Series;
  using Config = codem::Config;
  using ScalarType = codem::ScalarType;
  using Platform = codem::Platform;
  using BlobDeleter = codem::BlobDeleter;

  // ---- Config builder helpers ----
  using pluss::config; // from pluss/config.h
  namespace detail {
    using pluss::detail::make_config;
  }

  // --------------------------------------------------------------------------
  // Factories (create NEW Series). Ops on existing Series live as methods.
  // --------------------------------------------------------------------------

  inline Series empty(std::span<const std::int64_t> sizes, ScalarType dt) { return Series::empty(sizes, dt); }
  inline Series empty(std::initializer_list<std::int64_t> sizes, ScalarType dt) {
    return empty(std::span<const std::int64_t>(sizes.begin(), sizes.size()), dt);
  }
  inline Series empty(std::span<const std::int64_t> sizes, Config cfg) { return Series::empty(sizes, cfg.dtype); }
  inline Series empty(std::initializer_list<std::int64_t> sizes, Config cfg) {
    return empty(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg);
  }
  template <class... Opts>
  inline Series empty(std::span<const std::int64_t> sizes, Opts... opts) {
    return empty(sizes, detail::make_config(opts...));
  }
  template <class... Opts>
  inline Series empty(std::initializer_list<std::int64_t> sizes, Opts... opts) {
    return empty(std::span<const std::int64_t>(sizes.begin(), sizes.size()), detail::make_config(opts...));
  }
  template <class T>
  inline Series empty(std::span<const std::int64_t> sizes, std::initializer_list<std::pair<const char*, T>> kv) {
    return empty(sizes, pluss::config(kv));
  }

  inline Series zeros(std::span<const std::int64_t> sizes, ScalarType dt = ScalarType::Float32) { return Series::zeros(sizes, dt); }
  inline Series zeros(std::initializer_list<std::int64_t> sizes, ScalarType dt = ScalarType::Float32) {
    return zeros(std::span<const std::int64_t>(sizes.begin(), sizes.size()), dt);
  }
  inline Series zeros(std::span<const std::int64_t> sizes, Config cfg) { return Series::zeros(sizes, cfg.dtype); }
  inline Series zeros(std::initializer_list<std::int64_t> sizes, Config cfg) {
    return zeros(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg);
  }
  template <class... Opts>
  inline Series zeros(std::span<const std::int64_t> sizes, Opts... opts) {
    return zeros(sizes, detail::make_config(opts...));
  }
  template <class... Opts>
  inline Series zeros(std::initializer_list<std::int64_t> sizes, Opts... opts) {
    return zeros(std::span<const std::int64_t>(sizes.begin(), sizes.size()), detail::make_config(opts...));
  }
  template <class T>
  inline Series zeros(std::span<const std::int64_t> sizes, std::initializer_list<std::pair<const char*, T>> kv) {
    return zeros(sizes, pluss::config(kv));
  }

  inline Series ones(std::span<const std::int64_t> sizes, ScalarType dt = ScalarType::Float32) { return Series::ones(sizes, dt); }
  inline Series ones(std::initializer_list<std::int64_t> sizes, ScalarType dt = ScalarType::Float32) {
    return ones(std::span<const std::int64_t>(sizes.begin(), sizes.size()), dt);
  }
  inline Series ones(std::span<const std::int64_t> sizes, Config cfg) { return Series::ones(sizes, cfg.dtype); }
  inline Series ones(std::initializer_list<std::int64_t> sizes, Config cfg) {
    return ones(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg);
  }
  template <class... Opts>
  inline Series ones(std::span<const std::int64_t> sizes, Opts... opts) {
    return ones(sizes, detail::make_config(opts...));
  }
  template <class... Opts>
  inline Series ones(std::initializer_list<std::int64_t> sizes, Opts... opts) {
    return ones(std::span<const std::int64_t>(sizes.begin(), sizes.size()), detail::make_config(opts...));
  }
  template <class T>
  inline Series ones(std::span<const std::int64_t> sizes, std::initializer_list<std::pair<const char*, T>> kv) {
    return ones(sizes, pluss::config(kv));
  }

  inline Series full(std::span<const std::int64_t> sizes, double fill_value, ScalarType dt = ScalarType::Float32) { return Series::full(sizes, fill_value, dt); }
  inline Series full(std::initializer_list<std::int64_t> sizes, double fill_value, ScalarType dt = ScalarType::Float32) {
    return full(std::span<const std::int64_t>(sizes.begin(), sizes.size()), fill_value, dt);
  }
  inline Series full(std::span<const std::int64_t> sizes, double fill_value, Config cfg) { return Series::full(sizes, fill_value, cfg.dtype); }
  inline Series full(std::initializer_list<std::int64_t> sizes, double fill_value, Config cfg) {
    return full(std::span<const std::int64_t>(sizes.begin(), sizes.size()), fill_value, cfg);
  }
  template <class... Opts>
  inline Series full(std::span<const std::int64_t> sizes, double fill_value, Opts... opts) {
    return full(sizes, fill_value, detail::make_config(opts...));
  }
  template <class... Opts>
  inline Series full(std::initializer_list<std::int64_t> sizes, double fill_value, Opts... opts) {
    return full(std::span<const std::int64_t>(sizes.begin(), sizes.size()), fill_value, detail::make_config(opts...));
  }
  template <class T>
  inline Series full(std::span<const std::int64_t> sizes, double fill_value, std::initializer_list<std::pair<const char*, T>> kv) {
    return full(sizes, fill_value, pluss::config(kv));
  }

  // Scalar constants (factories)
  inline Series nan(ScalarType dt = ScalarType::Float32) { return codem::nan(dt); }
  inline Series inf(ScalarType dt = ScalarType::Float32) { return codem::inf(dt); }

  // Ranges / random
  inline Series arange(std::int64_t start, std::int64_t end, std::int64_t step = 1, ScalarType dt = ScalarType::Int64) { return Series::arange(start, end, step, dt); }
  inline Series linspace(double start, double end, std::int64_t steps, ScalarType dt = ScalarType::Float32) { return Series::linspace(start, end, steps, dt); }
  inline Series eye(std::int64_t n, std::int64_t m = -1, ScalarType dt = ScalarType::Float32) { return Series::eye(n, m, dt); }

  inline Series rand(std::span<const std::int64_t> sizes, ScalarType dt = ScalarType::Float32, std::uint64_t seed = 0) { return Series::rand(sizes, dt, seed); }
  inline Series rand(std::initializer_list<std::int64_t> sizes, ScalarType dt = ScalarType::Float32, std::uint64_t seed = 0) {
    return Series::rand(std::span<const std::int64_t>(sizes.begin(), sizes.size()), dt, seed);
  }
  inline Series rand(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed = 0) { return Series::rand(sizes, cfg.dtype, seed); }
  inline Series rand(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed = 0) {
    return Series::rand(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg.dtype, seed);
  }
  template <class... Opts>
  inline Series rand(std::span<const std::int64_t> sizes, std::uint64_t seed, Opts... opts) {
    return rand(sizes, detail::make_config(opts...), seed);
  }
  template <class... Opts>
  inline Series rand(std::initializer_list<std::int64_t> sizes, std::uint64_t seed, Opts... opts) {
    const Config cfg = detail::make_config(opts...);
    return Series::rand(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg.dtype, seed);
  }

  inline Series randn(std::span<const std::int64_t> sizes, ScalarType dt = ScalarType::Float32, std::uint64_t seed = 0) { return Series::randn(sizes, dt, seed); }
  inline Series randn(std::initializer_list<std::int64_t> sizes, ScalarType dt = ScalarType::Float32, std::uint64_t seed = 0) {
    return Series::randn(std::span<const std::int64_t>(sizes.begin(), sizes.size()), dt, seed);
  }
  inline Series randn(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed = 0) { return Series::randn(sizes, cfg.dtype, seed); }
  inline Series randn(std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed = 0) {
    return Series::randn(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg.dtype, seed);
  }
  template <class... Opts>
  inline Series randn(std::span<const std::int64_t> sizes, std::uint64_t seed, Opts... opts) {
    return randn(sizes, detail::make_config(opts...), seed);
  }
  template <class... Opts>
  inline Series randn(std::initializer_list<std::int64_t> sizes, std::uint64_t seed, Opts... opts) {
    const Config cfg = detail::make_config(opts...);
    return Series::randn(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg.dtype, seed);
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
    return randint(lo, hi, sizes, dt, seed);
  }
  inline Series randint(std::int64_t lo, std::int64_t hi, std::initializer_list<std::int64_t> sizes, Config cfg, std::uint64_t seed = 0) {
    return randint(lo, hi, std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
  }
  template <class... Opts>
  inline Series randint(std::int64_t lo, std::int64_t hi, std::span<const std::int64_t> sizes, std::uint64_t seed, Opts... opts) {
    return randint(lo, hi, sizes, detail::make_config(opts...), seed);
  }
  template <class... Opts>
  inline Series randint(std::int64_t lo, std::int64_t hi, std::initializer_list<std::int64_t> sizes, std::uint64_t seed, Opts... opts) {
    return randint(lo, hi, std::span<const std::int64_t>(sizes.begin(), sizes.size()), detail::make_config(opts...), seed);
  }

// interop with other libraries (e.g., Torch)
#ifdef PLUSS_ENABLE_TORCH
  inline Series from_torch(const ::torch::Tensor& t) { return codem::from_torch(t); }  // use to create a pluss::Series from a torch::Tensor (cheap copy of data)
  inline ::torch::Tensor to_torch(const Series& s) { return codem::to_torch(s); }
#endif

  // --------------------------------------------------------------------------
  // Value / blob / tensor-like factories (create NEW Series from user data)
  // --------------------------------------------------------------------------

  // Scalars:
  inline Series value(double v, ScalarType dtype = ScalarType::Float32) { return codem::value(v, dtype); }
  template <class T, std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>, int> = 0>
  inline Series value(T v, ScalarType dtype = ScalarType::Int64) { return codem::value(v, dtype); }
  inline Series value(bool v, ScalarType dtype = ScalarType::Bool) { return codem::value(v, dtype); }

  // Nested initializer_list: dtype auto-deduction when user doesn't specify.
  template <class T>
  inline Series value(std::initializer_list<T> list) {
    constexpr ScalarType dt = pluss::detail::dtype_from_init_list<std::initializer_list<T>>();
    return codem::value(list, dt);
  }
  template <class T>
  inline Series value(std::initializer_list<T> list, ScalarType dtype) {
    return codem::value(list, dtype);
  }

  // Compatibility factory: construct from an existing contiguous host buffer.
  // Prefer make_series(...) for explicit lvalue semantics.
  template <class T>
  inline Series value(const std::vector<T>& data, std::span<const std::int64_t> sizes, Config cfg = {}) {
    return codem::make_series<T>(data, sizes, cfg);
  }
  template <class T>
  inline Series value(const std::vector<T>& data, std::initializer_list<std::int64_t> sizes, Config cfg = {}) {
    return codem::make_series<T>(
      data,
      std::span<const std::int64_t>(sizes.begin(), sizes.size()),
      cfg
    );
  }

  // make_series(...): construct from an existing lvalue/range of host data.
  template <class T>
  inline Series make_series(const std::vector<T>& data, std::span<const std::int64_t> sizes, Config cfg = {}) {
    return codem::make_series<T>(data, sizes, cfg);
  }
  template <class T>
  inline Series make_series(const std::vector<T>& data, std::initializer_list<std::int64_t> sizes, Config cfg = {}) {
    return codem::make_series<T>(
      data,
      std::span<const std::int64_t>(sizes.begin(), sizes.size()),
      cfg
    );
  }
  template <class T>
  inline Series make_series(std::span<const T> data, std::span<const std::int64_t> sizes, Config cfg = {}) {
    return codem::make_series<T>(data, sizes, cfg);
  }
  template <class T>
  inline Series make_series(std::span<const T> data, std::initializer_list<std::int64_t> sizes, Config cfg = {}) {
    return codem::make_series<T>(data, sizes, cfg);
  }

  inline Series from_blob(void* data,
                          std::span<const std::int64_t> sizes,
                          std::span<const std::int64_t> strides,
                          BlobDeleter deleter,
                          ScalarType dtype = ScalarType::Float32) {
    return codem::from_blob(data, sizes, strides, deleter, dtype);
  }

} // namespace pluss
