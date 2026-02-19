#pragma once
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <span>
#include <vector>
#include <type_traits>
#include <iosfwd>

#include "CodeM/core/Errors.h"

#include "CodeM/core/SeriesBase.h"
#include "CodeM/core/DType.h"
#include "CodeM/core/exotic_containers.h"
#include "CodeM/core/Storage.h"
#include "CodeM/core/ScalarType.h"
#include "CodeM/core/Half.h"
#include "CodeM/core/Complex.h"
#include "CodeM/core/Scalar.h"
#include "MSharp/IntrusivePtr.h"
#include "pluss/export.h"
// Compatibility invoke layer (method dispatch helpers). Included here so the
// generated method surface can be included inside the class without bringing
// the compat header into class scope (which would introduce nested
// pluss::codem namespace declarations).
#include "CodeM/compat/Invoke.h"

#ifdef PLUSS_ENABLE_TORCH
#include <torch/torch.h>
#endif


namespace pluss::codem {

namespace ops_internal {
struct SeriesFactoryAccess;
} // namespace ops_internal

struct TernaryCompareBuilder;


// Forward declaration for tensor memory/layout format used by generated APIs.
// Full definition should come from the appropriate header if/when needed.
enum class MemoryFormat : std::uint8_t;

// User-facing Series (Torch-like Tensor).
// User-facing Series (Torch-like Tensor).
class CODEM_API Series : public SeriesBase {
public:
  template <class T>
  struct HostArray {
    std::vector<T> data;
    std::vector<std::int64_t> sizes;
    std::vector<std::int64_t> strides;
    bool contiguous{false};

    std::span<const std::int64_t> shape() const noexcept { return sizes; }
    std::span<const std::int64_t> stride_view() const noexcept { return strides; }
    std::int64_t dim() const noexcept { return static_cast<std::int64_t>(sizes.size()); }
    std::int64_t numel() const noexcept { return static_cast<std::int64_t>(data.size()); }
  };

  // Factory helpers (Torch-style free-functions live elsewhere; keep these for convenience).
  static Series undefined();
  static Series empty(std::span<const std::int64_t> sizes, ScalarType st);
  static Series empty(std::initializer_list<std::int64_t> sizes, ScalarType st) {
    return empty(std::span<const std::int64_t>(sizes.begin(), sizes.size()), st);
  }
  static Series zeros(std::span<const std::int64_t> sizes, ScalarType st);
  static Series zeros(std::initializer_list<std::int64_t> sizes, ScalarType st) {
    return zeros(std::span<const std::int64_t>(sizes.begin(), sizes.size()), st);
  }
  static Series ones(std::span<const std::int64_t> sizes, ScalarType st);
  static Series ones(std::initializer_list<std::int64_t> sizes, ScalarType st) {
    return ones(std::span<const std::int64_t>(sizes.begin(), sizes.size()), st);
  }
  static Series full(std::span<const std::int64_t> sizes, double value, ScalarType st);
  static Series full(std::initializer_list<std::int64_t> sizes, double value, ScalarType st) {
    return full(std::span<const std::int64_t>(sizes.begin(), sizes.size()), value, st);
  }
  static Series make_from_doubles(const std::vector<double>& flat, std::span<const std::int64_t> sizes, ScalarType st);

  // Core view/cast helpers used across the library.
  Series contiguous() const;
  Series to(ScalarType st) const;
  Series clone() const;
  Series platform(Platform target) const;
  TernaryCompareBuilder tri() const noexcept;

#ifdef PLUSS_ENABLE_TORCH
  ::torch::Tensor tensor() const;
#endif

  void* raw_mutable_data() noexcept { return reinterpret_cast<void*>(raw_data()); }

  // Materialize host data to a contiguous vector in row-major logical order.
  // For N-D shape-aware export, use to_host<T>().
  template <class T>
  std::vector<T> to_vec() const {
    return detail_materialize_vec_<T>();
  }

  template <class T>
  T to_vec(std::int64_t index) const {
    auto data = detail_materialize_vec_<T>();
    std::int64_t idx = index;
    if (idx < 0) {
      idx += static_cast<std::int64_t>(data.size());
    }
    if (idx < 0 || idx >= static_cast<std::int64_t>(data.size())) {
      throw_error("Series::to_vec<T>(index): index out of range");
    }
    return data[static_cast<std::size_t>(idx)];
  }

  // Materialize host data with full N-D metadata.
  template <class T>
  HostArray<T> to_host() const {
    HostArray<T> out;
    out.sizes.assign(sizes().begin(), sizes().end());
    out.strides.assign(strides().begin(), strides().end());
    out.contiguous = is_contiguous();
    out.data = detail_materialize_vec_<T>();
    return out;
  }

#if __has_include("CodeM/Series/SeriesOpsInternalMethods.inc")
  #include "CodeM/Series/SeriesOpsInternalMethods.inc"
#endif


// ---------------------------------------------------------------------------
// Full op surface
// ---------------------------------------------------------------------------
// The bulk of the Torch-like API (generated wrappers + operator sugar) lives in
// a separate header to avoid poisoning light includes.
//
// - Most internal code should include SeriesBase.h or SeriesSlim.h.
// - End-user code should include Series.h (default, full surface).
//
// To forbid pulling in the full surface from a translation unit, define
// CODEM_ASSERT_NO_SERIES_SURFACE before including Series.h (or include
// SeriesSlim.h).
#if !defined(CODEM_NO_SERIES_SURFACE)
  #include "CodeM/core/SeriesSurface.h"
#endif


  // ---- Core reductions / a small torch-compatible surface ----
  // These are depended on internally (e.g. norm implementation) and are implemented
  // in Series.cpp / OpsInternal.cpp

  // Variance / std (dim overloads are used internally but not yet codegen'd).
  Series var() const { return var(true); }
  Series var(std::int64_t dim, bool unbiased=true, bool keepdim=false) const;
  Series std() const { return std(true); }
  Series std(std::int64_t dim, bool unbiased=true, bool keepdim=false) const;

  // Norm overloads used throughout the codebase.
  Series norm(double p, std::int64_t dim, bool keepdim=false) const;

  // PlussQit naming deviations (method-first aliases).
  Series pile(const std::vector<Series>& others, std::int64_t dim=0) const;
  Series unite(const std::vector<Series>& others, std::int64_t dim=0) const;

private:
  template <class>
  struct detail_false_ : std::false_type {};

  template <class T>
  std::vector<T> detail_materialize_vec_() const {
    const ScalarType want = detail_scalar_type_for_<T>();
    if (scalar_type() != want) {
      // User-facing extraction supports dtype conversion (promotion/demotion)
      // through Series::to(...). If a conversion is unsupported, Series::to
      // throws with a detailed error.
      return to(want).template detail_materialize_vec_<T>();
    }

    std::vector<T> out(static_cast<std::size_t>(numel()));
    if (out.empty()) {
      return out;
    }

    if (is_contiguous()) {
      if constexpr (std::is_same_v<std::remove_cv_t<T>, bool>) {
        const auto* base = reinterpret_cast<const bool*>(raw_data());
        for (std::size_t i = 0; i < out.size(); ++i) {
          out[i] = base[i];
        }
      } else {
        std::memcpy(out.data(), raw_data(), out.size() * sizeof(T));
      }
      return out;
    }

    const auto* base = reinterpret_cast<const T*>(raw_data());
    const std::vector<std::int64_t> sz(sizes().begin(), sizes().end());
    const std::vector<std::int64_t> st(strides().begin(), strides().end());
    const std::int64_t rank = static_cast<std::int64_t>(sz.size());

    if (rank == 0) {
      out[0] = base[0];
      return out;
    }

    for (std::int64_t lin = 0; lin < numel(); ++lin) {
      std::int64_t rem = lin;
      std::int64_t off = 0;
      for (std::int64_t d = rank - 1; d >= 0; --d) {
        const std::int64_t dim = sz[static_cast<std::size_t>(d)];
        const std::int64_t idx = rem % dim;
        rem /= dim;
        off += idx * st[static_cast<std::size_t>(d)];
      }
      out[static_cast<std::size_t>(lin)] = base[off];
    }

    return out;
  }

  template <class T>
  static ScalarType detail_scalar_type_for_() {
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
    } else if constexpr (std::is_same_v<U, Float16>) {
      return ScalarType::Float16;
    } else if constexpr (std::is_same_v<U, BFloat16>) {
      return ScalarType::BFloat16;
    } else if constexpr (std::is_same_v<U, Complex64>) {
      return ScalarType::Complex64;
    } else if constexpr (std::is_same_v<U, Complex128>) {
      return ScalarType::Complex128;
    } else {
      static_assert(detail_false_<U>::value, "Series::to_vec<T>: unsupported element type");
    }
  }

  Series() = default;
  // Internal-only ctor: Series objects are created via factories and internal
  // kernels (ops_internal), not by end users.
  explicit Series(pluss::msharp::intrusive_ptr<SeriesImpl> impl)
      : SeriesBase(std::move(impl)) {}

  // Internal construction hook used by ops_internal implementations.
  friend struct ops_internal::SeriesFactoryAccess;
};


} // namespace pluss::codem

// Now that `pluss::codem::Series` is a complete type, include the compat
// header again to compile the template helper definitions that rely on the
// complete `Series` definition. The compat header intentionally exposes the
// templates only when `PLUSS_CODEM_SERIES_COMPLETE` is defined on a second
// include.
#define PLUSS_CODEM_SERIES_COMPLETE
#include "CodeM/compat/Invoke.h"
#undef PLUSS_CODEM_SERIES_COMPLETE
