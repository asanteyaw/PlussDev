#pragma once

#include "pluss/config.h"
#include "pluss/codem/codem.h"
#include "pluss/codem/series_options.h"
#include "pluss/detail/factory_deduction.h"
#include "pluss/detail/nested_series_literal.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <limits>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <span>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace pluss::detail {

template <class T>
inline constexpr bool always_false_v = false;

struct DataReader final {
  enum class ContainerType : std::uint8_t { Scalar, InitList, HostBuffer, Series };
  enum class DefaultDTypePolicy : std::uint8_t { PreserveInput, TorchLike };
  using DTypeHook = ScalarType (*)(ScalarType);
  struct Option final {
    enum class Kind : std::uint8_t { None, DType, Platform, Config, Shape };

    Option() = default;
    Option(ScalarType dt) : kind(Kind::DType), dtype(dt), dtype_set(true) {}
    Option(Platform p) : kind(Kind::Platform), platform(p), platform_set(true) {}
    Option(const Config& c)
        : kind(Kind::Config), dtype(c.dtype), platform(c.platform), dtype_set(true), platform_set(true), config(c) {}
    Option(Config&& c)
        : kind(Kind::Config),
          dtype(c.dtype),
          platform(c.platform),
          dtype_set(true),
          platform_set(true),
          config(std::move(c)) {}
    Option(cfg::DType o) : kind(Kind::DType), dtype(o.v), dtype_set(true) {}
    Option(cfg::Platform o) : kind(Kind::Platform), platform(o.v), platform_set(true) {}
    Option(cfg::Pinned) : kind(Kind::None) {}
    Option(cfg::TracksSlope o) : kind(Kind::None), tracks_slope_set(true), tracks_slope(o.v) {}
    Option(cfg::RetainSensitivity o)
        : kind(Kind::None), retain_sensitivity_set(true), retain_sensitivity(o.v) {}
    Option(const SeriesStorageOptions& o)
        : kind(Kind::None),
          dtype(o.dtype.value_or(ScalarType::Float32)),
          platform(o.platform.value_or(Platform::CPU)),
          dtype_set(o.dtype.has_value()),
          platform_set(o.platform.has_value()) {
    }
    Option(const SeriesAutodiffOptions& o)
        : kind(Kind::None),
          tracks_slope_set(o.tracks_slope.has_value()),
          tracks_slope(o.tracks_slope.value_or(false)),
          retain_sensitivity_set(o.retain_sensitivity.has_value()),
          retain_sensitivity(o.retain_sensitivity.value_or(false)) {
    }
    Option(const SeriesOptions& o)
        : Option(o.storage_options()) {
      tracks_slope_set = o.autodiff_options().tracks_slope.has_value();
      tracks_slope = o.autodiff_options().tracks_slope.value_or(false);
      retain_sensitivity_set = o.autodiff_options().retain_sensitivity.has_value();
      retain_sensitivity = o.autodiff_options().retain_sensitivity.value_or(false);
    }
    Option(std::span<const std::int64_t> s) : kind(Kind::Shape), sizes(s.begin(), s.end()) {}
    Option(std::initializer_list<std::int64_t> s) : kind(Kind::Shape), sizes(s.begin(), s.end()) {}
    Option(const std::vector<std::int64_t>& s) : kind(Kind::Shape), sizes(s.begin(), s.end()) {}
    Option(std::vector<std::int64_t>&& s) : kind(Kind::Shape), sizes(std::move(s)) {}
    Option(const std::unordered_map<std::string, std::string>& m)
        : kind(Kind::Config), config(pluss::config(m)) {
      dtype = config.dtype;
      platform = config.platform;
      dtype_set = true;
      platform_set = true;
    }
    template <class T>
    Option(std::initializer_list<std::pair<const char*, T>> kv)
        : kind(Kind::Config), config(pluss::config(kv)) {
      dtype = config.dtype;
      platform = config.platform;
      dtype_set = true;
      platform_set = true;
    }

    Kind kind{Kind::None};
    ScalarType dtype{ScalarType::Float32};
    Platform platform{Platform::CPU};
    bool dtype_set{false};
    bool platform_set{false};
    Config config{};
    std::vector<std::int64_t> sizes{};
    bool tracks_slope_set{false};
    bool tracks_slope{false};
    bool retain_sensitivity_set{false};
    bool retain_sensitivity{false};
  };

  DataReader() = default;

  template <class T, std::enable_if_t<std::is_arithmetic_v<std::remove_cv_t<T>>, int> = 0>
  DataReader(T v) {
    assign_scalar_(v);
  }

  template <class T>
  DataReader(std::initializer_list<T> list) {
    assign_init_list_(list);
  }

  DataReader(std::initializer_list<NestedSeriesLiteral> list) {
    assign_nested_(list);
  }

  // Supports the `{68.0, std::nullopt, 72.0}` null-gap construction pattern for
  // floating-point data. `std::nullopt` maps to NaN, the existing null-sentinel
  // convention for raw (validity-mask-less) numeric Series/Thread values. Deliberately
  // NOT templated on T: template argument deduction for `std::initializer_list<T>`
  // requires every element to independently deduce the *same* T, which a mix of
  // `double` and `std::nullopt_t` elements can never satisfy. Making the element type
  // concrete (`std::optional<double>`) instead lets each element go through ordinary
  // copy-initialization, where the implicit `double`/`std::nullopt_t` -> `optional<double>`
  // conversions are allowed.
  DataReader(std::initializer_list<std::optional<double>> list) {
    assign_optional_double_list_(list);
  }

  template <class T>
  DataReader(std::span<const T> data) {
    assign_host_span_(data);
  }

  template <class T>
  DataReader(std::span<const T> data, std::span<const std::int64_t> sizes) {
    assign_host_span_(data);
    with_sizes(sizes);
  }

  template <class T>
  DataReader(std::span<const T> data, std::initializer_list<std::int64_t> sizes) {
    assign_host_span_(data);
    with_sizes(sizes);
  }

  template <class T>
  DataReader(const std::vector<T>& data) {
    assign_host_vector_(data);
  }

  template <class T>
  DataReader(const std::vector<T>& data, std::span<const std::int64_t> sizes) {
    assign_host_vector_(data);
    with_sizes(sizes);
  }

  template <class T>
  DataReader(const std::vector<T>& data, std::initializer_list<std::int64_t> sizes) {
    assign_host_vector_(data);
    with_sizes(sizes);
  }

  template <class T, std::size_t N>
  DataReader(const std::array<T, N>& data) {
    assign_host_span_(std::span<const T>(data.data(), data.size()));
  }

  template <class T, std::size_t N>
  DataReader(const std::array<T, N>& data, std::span<const std::int64_t> sizes) {
    assign_host_span_(std::span<const T>(data.data(), data.size()));
    with_sizes(sizes);
  }

  template <class T, std::size_t N>
  DataReader(const std::array<T, N>& data, std::initializer_list<std::int64_t> sizes) {
    assign_host_span_(std::span<const T>(data.data(), data.size()));
    with_sizes(sizes);
  }

  template <class T, std::size_t N>
  DataReader(const T (&data)[N]) {
    assign_host_span_(std::span<const T>(data, N));
  }

  template <class T, std::size_t N>
  DataReader(const T (&data)[N], std::span<const std::int64_t> sizes) {
    assign_host_span_(std::span<const T>(data, N));
    with_sizes(sizes);
  }

  template <class T, std::size_t N>
  DataReader(const T (&data)[N], std::initializer_list<std::int64_t> sizes) {
    assign_host_span_(std::span<const T>(data, N));
    with_sizes(sizes);
  }

  template <class T>
  DataReader(const T* data, std::int64_t count) {
    PLUSS_CHECK(data != nullptr || count == 0, "DataReader: null pointer with non-zero count");
    PLUSS_CHECK(count >= 0, "DataReader: count must be non-negative");
    assign_host_span_(std::span<const T>(data, static_cast<std::size_t>(count)));
  }

  template <class T>
  DataReader(const T* data, std::int64_t count, std::span<const std::int64_t> sizes) {
    PLUSS_CHECK(data != nullptr || count == 0, "DataReader: null pointer with non-zero count");
    PLUSS_CHECK(count >= 0, "DataReader: count must be non-negative");
    assign_host_span_(std::span<const T>(data, static_cast<std::size_t>(count)));
    with_sizes(sizes);
  }

  template <class T>
  DataReader(const T* data, std::int64_t count, std::initializer_list<std::int64_t> sizes) {
    PLUSS_CHECK(data != nullptr || count == 0, "DataReader: null pointer with non-zero count");
    PLUSS_CHECK(count >= 0, "DataReader: count must be non-negative");
    assign_host_span_(std::span<const T>(data, static_cast<std::size_t>(count)));
    with_sizes(sizes);
  }

  DataReader(const Series& s) {
    assign_series_(s);
  }

  DataReader(Series&& s) {
    assign_series_move_(std::move(s));
  }

  bool is_scalar() const noexcept { return type_ == ContainerType::Scalar; }
  bool is_init_list() const noexcept { return type_ == ContainerType::InitList; }
  bool is_host_buffer() const noexcept { return type_ == ContainerType::HostBuffer; }
  bool is_series() const noexcept { return type_ == ContainerType::Series; }
  ScalarType scalar_type() const noexcept { return source_dtype_; }
  std::span<const std::int64_t> sizes() const noexcept { return sizes_; }
  const std::string& debug_repr() const noexcept { return debug_repr_; }

  static void set_default_dtype_policy(DefaultDTypePolicy policy) noexcept { dtype_policy_ref_() = policy; }
  static DefaultDTypePolicy default_dtype_policy() noexcept { return dtype_policy_ref_(); }

  static void set_default_float_dtype(ScalarType dtype) {
    PLUSS_CHECK(is_float_dtype_(dtype), "DataReader::set_default_float_dtype: expected floating dtype");
    default_float_dtype_ref_() = dtype;
  }
  static ScalarType default_float_dtype() noexcept { return default_float_dtype_ref_(); }

  static void set_dtype_hook(DTypeHook hook) noexcept { dtype_hook_ref_() = hook; }
  static DTypeHook dtype_hook() noexcept { return dtype_hook_ref_(); }

  static void set_strict_mixed_init_list(bool enabled) noexcept { strict_mixed_init_list_ref_() = enabled; }
  static bool strict_mixed_init_list() noexcept { return strict_mixed_init_list_ref_(); }

  DataReader& with_sizes(std::span<const std::int64_t> sizes) {
    if (type_ == ContainerType::HostBuffer) {
      sizes_.assign(sizes.begin(), sizes.end());
      return *this;
    }
    PLUSS_CHECK(
        same_shape_(sizes_, sizes),
        "DataReader::with_sizes: explicit sizes are only supported for host-buffer inputs");
    return *this;
  }

  DataReader& with_sizes(std::initializer_list<std::int64_t> sizes) {
    return with_sizes(std::span<const std::int64_t>(sizes.begin(), sizes.size()));
  }

  DataReader& with_dtype(ScalarType dtype) noexcept {
    dtype_override_ = dtype;
    dtype_override_set_ = true;
    return *this;
  }

  DataReader& with_platform(Platform platform) noexcept {
    platform_override_ = platform;
    platform_override_set_ = true;
    return *this;
  }

  DataReader& with_config(Config cfg) noexcept {
    if (cfg.dtype != ScalarType::Float32) {
      with_dtype(cfg.dtype);
    }
    with_platform(cfg.platform);
    return *this;
  }

  DataReader& with_series_storage_options(const SeriesStorageOptions& options) noexcept {
    if (options.dtype.has_value()) with_dtype(*options.dtype);
    if (options.platform.has_value()) with_platform(*options.platform);
    return *this;
  }

  Series to_series() const {
    Series out = (type_ == ContainerType::HostBuffer) ? materialize_host_() : staged_cpu_;
    PLUSS_CHECK(out.defined(), "DataReader: undefined staged value");
    return (type_ == ContainerType::HostBuffer) ? std::move(out) : cast_and_move_(std::move(out));
  }

  template <class... Opts>
  Series to_series(Option first, Opts&&... opts) const {
    DataReader copy = *this;
    copy.apply_option_(first);
    (copy.apply_option_(std::forward<Opts>(opts)), ...);
    return copy.to_series();
  }

  template <class... Opts>
  Series to_series(Opts&&... opts) const {
    DataReader copy = *this;
    (copy.apply_option_(std::forward<Opts>(opts)), ...);
    return copy.to_series();
  }

  void pretty_print_recursive(std::ostream& os) const { os << debug_repr_; }

 private:
  static DefaultDTypePolicy& dtype_policy_ref_() {
    static DefaultDTypePolicy policy = DefaultDTypePolicy::PreserveInput;
    return policy;
  }

  static ScalarType& default_float_dtype_ref_() {
    static ScalarType dtype = ScalarType::Float32;
    return dtype;
  }

  static DTypeHook& dtype_hook_ref_() {
    static DTypeHook hook = nullptr;
    return hook;
  }

  static bool& strict_mixed_init_list_ref_() {
    static bool strict = false;
    return strict;
  }

  static bool is_integer_dtype_(ScalarType dt) {
    switch (dt) {
      case ScalarType::UInt8:
      case ScalarType::UInt16:
      case ScalarType::UInt32:
      case ScalarType::UInt64:
      case ScalarType::Int8:
      case ScalarType::Int16:
      case ScalarType::Int32:
      case ScalarType::Int64:
        return true;
      default:
        return false;
    }
  }

  static bool is_float_dtype_(ScalarType dt) {
    switch (dt) {
      case ScalarType::Float16:
      case ScalarType::BFloat16:
      case ScalarType::Float32:
      case ScalarType::Float64:
        return true;
      default:
        return false;
    }
  }

  static bool is_complex_dtype_(ScalarType dt) {
    return dt == ScalarType::Complex64 || dt == ScalarType::Complex128;
  }

  static const char* scalar_type_name_(ScalarType dt) {
    switch (dt) {
      case ScalarType::Bool: return "bool";
      case ScalarType::UInt8: return "uint8";
      case ScalarType::UInt16: return "uint16";
      case ScalarType::UInt32: return "uint32";
      case ScalarType::UInt64: return "uint64";
      case ScalarType::Int8: return "int8";
      case ScalarType::Int16: return "int16";
      case ScalarType::Int32: return "int32";
      case ScalarType::Int64: return "int64";
      case ScalarType::Float16: return "float16";
      case ScalarType::BFloat16: return "bfloat16";
      case ScalarType::Float32: return "float32";
      case ScalarType::Float64: return "float64";
      case ScalarType::Complex64: return "complex64";
      case ScalarType::Complex128: return "complex128";
      default: return "unknown";
    }
  }

  static const char* platform_name_(Platform p) {
    switch (p) {
      case Platform::CPU: return "cpu";
      case Platform::CUDA: return "cuda";
      case Platform::MPS: return "mps";
      default: return "unknown";
    }
  }

  static std::string shape_string_(std::span<const std::int64_t> shape) {
    std::ostringstream oss;
    oss << "[";
    for (std::size_t i = 0; i < shape.size(); ++i) {
      if (i) oss << ", ";
      oss << shape[i];
    }
    oss << "]";
    return oss.str();
  }

  template <class T>
  static std::string scalar_repr_(const T& v) {
    if constexpr (std::is_same_v<std::remove_cv_t<T>, bool>) {
      return v ? "true" : "false";
    } else {
      std::ostringstream oss;
      oss << v;
      return oss.str();
    }
  }

  template <class T>
  struct is_init_list_ : std::false_type {};
  template <class U>
  struct is_init_list_<std::initializer_list<U>> : std::true_type {};

  template <class T>
  static std::string render_value_(const T& value) {
    if constexpr (is_init_list_<std::remove_cv_t<T>>::value) {
      std::ostringstream oss;
      oss << "{";
      bool first = true;
      for (const auto& elem : value) {
        if (!first) oss << ", ";
        first = false;
        oss << render_value_(elem);
      }
      oss << "}";
      return oss.str();
    } else {
      return scalar_repr_(value);
    }
  }

  static std::string render_nested_literal_(const NestedSeriesLiteral& node) {
    if (node.kind == NestedSeriesLiteral::NodeKind::Scalar) {
      switch (node.leaf_kind) {
        case LiteralLeafKind::Bool:
          return node.scalar != 0.0 ? "true" : "false";
        case LiteralLeafKind::UInt8:
        case LiteralLeafKind::Int: {
          const auto iv = static_cast<std::int64_t>(node.scalar);
          return scalar_repr_(iv);
        }
        case LiteralLeafKind::Float32:
        case LiteralLeafKind::Float64: {
          return scalar_repr_(node.scalar);
        }
      }
    }
    std::ostringstream oss;
    oss << "{";
    for (std::size_t i = 0; i < node.children.size(); ++i) {
      if (i) oss << ", ";
      oss << render_nested_literal_(node.children[i]);
    }
    oss << "}";
    return oss.str();
  }

  template <class T>
  static std::string render_host_preview_(std::span<const T> data) {
    std::ostringstream oss;
    oss << "{";
    const std::size_t limit = std::min<std::size_t>(8, data.size());
    for (std::size_t i = 0; i < limit; ++i) {
      if (i) oss << ", ";
      oss << scalar_repr_(data[i]);
    }
    if (data.size() > limit) oss << ", ...";
    oss << "}";
    return oss.str();
  }

  static std::string literal_stats_string_(const LiteralStats& stats) {
    std::vector<const char*> kinds;
    if (stats.has_bool) kinds.push_back("bool");
    if (stats.has_uint8) kinds.push_back("uint8");
    if (stats.has_int) kinds.push_back("int");
    if (stats.has_float32) kinds.push_back("float32");
    if (stats.has_float64) kinds.push_back("float64");
    std::ostringstream oss;
    for (std::size_t i = 0; i < kinds.size(); ++i) {
      if (i) oss << ", ";
      oss << kinds[i];
    }
    return oss.str();
  }

  static bool literal_is_mixed_(const LiteralStats& stats) {
    int count = 0;
    count += stats.has_bool ? 1 : 0;
    count += stats.has_uint8 ? 1 : 0;
    count += stats.has_int ? 1 : 0;
    count += stats.has_float32 ? 1 : 0;
    count += stats.has_float64 ? 1 : 0;
    return count > 1;
  }

  static ScalarType policy_dtype_(ScalarType source) {
    if (auto hook = dtype_hook_ref_()) {
      return hook(source);
    }
    ScalarType dt = source;
    if (dtype_policy_ref_() == DefaultDTypePolicy::PreserveInput) return dt;
    if (is_integer_dtype_(dt)) {
      return ScalarType::Int64;
    }
    if (is_float_dtype_(dt)) {
      return default_float_dtype_ref_();
    }
    if (is_complex_dtype_(dt)) {
      return default_float_dtype_ref_() == ScalarType::Float64 ? ScalarType::Complex128
                                                               : ScalarType::Complex64;
    }
    return dt;
  }

  template <class T>
  static constexpr ScalarType default_scalar_dtype_for_() {
    using U = std::remove_cv_t<T>;
    if constexpr (std::is_same_v<U, bool>) {
      return ScalarType::Bool;
    } else if constexpr (std::is_floating_point_v<U>) {
      return std::is_same_v<U, double> ? ScalarType::Float64 : ScalarType::Float32;
    } else if constexpr (std::is_integral_v<U>) {
      return std::is_same_v<U, std::uint8_t> ? ScalarType::UInt8 : ScalarType::Int64;
    } else {
      return ScalarType::Float32;
    }
  }

  template <class T>
  static constexpr ScalarType scalar_type_for_host_() {
    using U = std::remove_cv_t<T>;
    if constexpr (std::is_same_v<U, bool>) {
      return ScalarType::Bool;
    } else if constexpr (std::is_same_v<U, std::uint8_t>) {
      return ScalarType::UInt8;
    } else if constexpr (std::is_same_v<U, std::uint16_t>) {
      return ScalarType::UInt16;
    } else if constexpr (std::is_same_v<U, std::uint32_t>) {
      return ScalarType::UInt32;
    } else if constexpr (std::is_same_v<U, std::uint64_t>) {
      return ScalarType::UInt64;
    } else if constexpr (std::is_same_v<U, std::int8_t>) {
      return ScalarType::Int8;
    } else if constexpr (std::is_same_v<U, std::int16_t>) {
      return ScalarType::Int16;
    } else if constexpr (std::is_same_v<U, std::int32_t>) {
      return ScalarType::Int32;
    } else if constexpr (std::is_same_v<U, std::int64_t>) {
      return ScalarType::Int64;
    } else if constexpr (std::is_same_v<U, float>) {
      return ScalarType::Float32;
    } else if constexpr (std::is_same_v<U, double>) {
      return ScalarType::Float64;
    } else if constexpr (std::is_same_v<U, codem::Float16>) {
      return ScalarType::Float16;
    } else if constexpr (std::is_same_v<U, codem::BFloat16>) {
      return ScalarType::BFloat16;
    } else if constexpr (std::is_same_v<U, codem::Complex64>) {
      return ScalarType::Complex64;
    } else if constexpr (std::is_same_v<U, codem::Complex128>) {
      return ScalarType::Complex128;
    } else {
      static_assert(always_false_v<U>, "DataReader: unsupported host-buffer element type");
      return ScalarType::Float32;
    }
  }

  void apply_option_(Option opt) {
    if (opt.dtype_set) with_dtype(opt.dtype);
    if (opt.platform_set) with_platform(opt.platform);
    switch (opt.kind) {
      case Option::Kind::None:
        return;
      case Option::Kind::DType:
        with_dtype(opt.dtype);
        return;
      case Option::Kind::Platform:
        with_platform(opt.platform);
        return;
      case Option::Kind::Config:
        with_config(opt.config);
        return;
      case Option::Kind::Shape:
        with_sizes(std::span<const std::int64_t>(opt.sizes.data(), opt.sizes.size()));
        return;
    }
  }

  void apply_option_(ScalarType dt) { with_dtype(dt); }
  void apply_option_(Platform p) { with_platform(p); }
  void apply_option_(Config& cfg) { with_config(cfg); }
  void apply_option_(const Config& cfg) { with_config(cfg); }
  void apply_option_(Config&& cfg) { with_config(cfg); }
  void apply_option_(cfg::DType o) { with_dtype(o.v); }
  void apply_option_(cfg::Platform o) { with_platform(o.v); }
  void apply_option_(cfg::Pinned) {}
  void apply_option_(cfg::TracksSlope) {}
  void apply_option_(cfg::RetainSensitivity) {}
  void apply_option_(const SeriesStorageOptions& options) { with_series_storage_options(options); }
  void apply_option_(const SeriesAutodiffOptions&) {}
  void apply_option_(const SeriesOptions& options) { with_series_storage_options(options.storage_options()); }
  void apply_option_(std::span<const std::int64_t> sizes) { with_sizes(sizes); }
  void apply_option_(std::initializer_list<std::int64_t> sizes) { with_sizes(sizes); }
  void apply_option_(const std::unordered_map<std::string, std::string>& m) { with_config(pluss::config(m)); }
  template <class T>
  void apply_option_(std::initializer_list<std::pair<const char*, T>> kv) {
    with_config(pluss::config(kv));
  }
  template <class T>
  void apply_option_(T&&) {
    static_assert(always_false_v<std::remove_cvref_t<T>>, "Unsupported DataReader option");
  }

  template <class T>
  void assign_scalar_(T v) {
    type_ = ContainerType::Scalar;
    source_dtype_ = default_scalar_dtype_for_<T>();
    if constexpr (std::is_same_v<std::remove_cv_t<T>, bool>) {
      staged_cpu_ = codem::value(v, source_dtype_);
    } else if constexpr (std::is_floating_point_v<T>) {
      staged_cpu_ = codem::value(static_cast<double>(v), source_dtype_);
    } else {
      staged_cpu_ = codem::value(v, source_dtype_);
    }
    sizes_.clear();
    element_count_ = 1;
    debug_repr_ = scalar_repr_(v);
  }

  template <class T>
  void assign_init_list_(std::initializer_list<T> list) {
    type_ = ContainerType::InitList;
    source_dtype_ = dtype_from_init_list<std::initializer_list<T>>();
    staged_cpu_ = codem::value(list, source_dtype_);
    sizes_.assign(staged_cpu_.sizes().begin(), staged_cpu_.sizes().end());
    element_count_ = static_cast<std::int64_t>(staged_cpu_.numel());
    debug_repr_ = render_value_(list);
  }

  void assign_nested_(std::initializer_list<NestedSeriesLiteral> list) {
    NestedSeriesLiteral root(list);
    if (strict_mixed_init_list_ref_()) {
      LiteralStats stats;
      collect_literal_stats(root, stats);
      if (literal_is_mixed_(stats)) {
        pluss::codem::throw_error(
            "DataReader: mixed scalar kinds in nested initializer_list: " + literal_stats_string_(stats));
      }
    }
    type_ = ContainerType::InitList;
    staged_cpu_ = pluss::detail::to_series(root);
    source_dtype_ = staged_cpu_.scalar_type();
    sizes_.assign(staged_cpu_.sizes().begin(), staged_cpu_.sizes().end());
    element_count_ = static_cast<std::int64_t>(staged_cpu_.numel());
    debug_repr_ = render_nested_literal_(root);
  }

  template <class T>
  void assign_host_span_(std::span<const T> data) {
    using U = std::remove_cv_t<T>;
    type_ = ContainerType::HostBuffer;
    source_dtype_ = scalar_type_for_host_<U>();
    element_count_ = static_cast<std::int64_t>(data.size());
    sizes_ = {element_count_};
    host_bytes_.resize(static_cast<std::size_t>(element_count_) * sizeof(U));
    if (!data.empty()) {
      std::memcpy(host_bytes_.data(), data.data(), host_bytes_.size());
    }
    debug_repr_ = render_host_preview_(data);
  }

  template <class T>
  void assign_host_vector_(const std::vector<T>& data) {
    using U = std::remove_cv_t<T>;
    if constexpr (std::is_same_v<U, bool>) {
      type_ = ContainerType::HostBuffer;
      source_dtype_ = ScalarType::Bool;
      element_count_ = static_cast<std::int64_t>(data.size());
      sizes_ = {element_count_};
      host_bytes_.resize(static_cast<std::size_t>(element_count_));
      for (std::size_t i = 0; i < data.size(); ++i) {
        host_bytes_[i] = static_cast<std::byte>(data[i] ? 1 : 0);
      }
      std::ostringstream oss;
      oss << "{";
      const std::size_t limit = std::min<std::size_t>(8, data.size());
      for (std::size_t i = 0; i < limit; ++i) {
        if (i) oss << ", ";
        oss << (data[i] ? "true" : "false");
      }
      if (data.size() > limit) oss << ", ...";
      oss << "}";
      debug_repr_ = oss.str();
    } else {
      assign_host_span_(std::span<const U>(data.data(), data.size()));
    }
  }

  void assign_optional_double_list_(std::initializer_list<std::optional<double>> list) {
    std::vector<double> values;
    values.reserve(list.size());
    for (const auto& v : list) {
      values.push_back(v.has_value() ? *v : std::numeric_limits<double>::quiet_NaN());
    }
    assign_host_vector_(values);
  }

  void assign_series_(const Series& s) {
    PLUSS_CHECK(s.defined(), "DataReader: cannot ingest undefined Series");
    type_ = ContainerType::Series;
    source_dtype_ = s.scalar_type();
    staged_cpu_ = s.platform() == Platform::CPU ? s : s.to(Platform::CPU);
    sizes_.assign(staged_cpu_.sizes().begin(), staged_cpu_.sizes().end());
    element_count_ = static_cast<std::int64_t>(staged_cpu_.numel());
    debug_repr_ = "Series(shape=" + shape_string_(sizes_) + ", dtype=" + scalar_type_name_(source_dtype_) +
                  ", platform=" + platform_name_(s.platform()) + ")";
  }

  void assign_series_move_(Series&& s) {
    PLUSS_CHECK(s.defined(), "DataReader: cannot ingest undefined Series");
    type_ = ContainerType::Series;
    const Platform src_platform = s.platform();
    source_dtype_ = s.scalar_type();
    staged_cpu_ = s.platform() == Platform::CPU ? std::move(s) : s.to(Platform::CPU);
    sizes_.assign(staged_cpu_.sizes().begin(), staged_cpu_.sizes().end());
    element_count_ = static_cast<std::int64_t>(staged_cpu_.numel());
    debug_repr_ = "Series(shape=" + shape_string_(sizes_) + ", dtype=" + scalar_type_name_(source_dtype_) +
                  ", platform=" + platform_name_(src_platform) + ")";
  }

  static bool same_shape_(
      std::span<const std::int64_t> a,
      std::span<const std::int64_t> b) {
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
  }

  static std::int64_t numel_from_sizes_(std::span<const std::int64_t> sizes) {
    std::int64_t n = 1;
    for (const auto s : sizes) {
      PLUSS_CHECK(s >= 0, "DataReader: sizes must be non-negative");
      n *= s;
    }
    return n;
  }

  ScalarType target_dtype_() const noexcept {
    if (dtype_override_set_) return dtype_override_;
    if (type_ == ContainerType::Series) return source_dtype_;
    return policy_dtype_(source_dtype_);
  }

  Platform target_platform_() const noexcept {
    return platform_override_set_ ? platform_override_ : Platform::CPU;
  }

  Series cast_and_move_(Series out) const {
    const ScalarType dt = target_dtype_();
    const Platform pf = target_platform_();
    if (out.scalar_type() != dt) out = out.to(dt);
    if (out.platform() != pf) out = out.to(pf);
    return out;
  }

  Series materialize_host_() const {
    PLUSS_CHECK(element_count_ >= 0, "DataReader: invalid host element count");
    const auto shape = std::span<const std::int64_t>(sizes_.data(), sizes_.size());
    PLUSS_CHECK(
        numel_from_sizes_(shape) == element_count_,
        "DataReader: host-buffer element count does not match provided sizes");
    Series out = Series::empty(shape, source_dtype_, Platform::CPU).contiguous();
    if (!host_bytes_.empty()) {
      std::memcpy(out.raw_data(), host_bytes_.data(), host_bytes_.size());
    }
    return cast_and_move_(std::move(out));
  }

  ContainerType type_{ContainerType::Scalar};
  Series staged_cpu_{Series::undefined()};
  ScalarType source_dtype_{ScalarType::Float32};
  std::vector<std::int64_t> sizes_{};
  std::int64_t element_count_{0};
  std::vector<std::byte> host_bytes_{};
  bool dtype_override_set_{false};
  ScalarType dtype_override_{ScalarType::Float32};
  bool platform_override_set_{false};
  Platform platform_override_{Platform::CPU};
  std::string debug_repr_{"{}"};
};

inline std::ostream& operator<<(std::ostream& os, const DataReader& reader) {
  reader.pretty_print_recursive(os);
  return os;
}

} // namespace pluss::detail

namespace pluss {
using DataReader = detail::DataReader;
}
