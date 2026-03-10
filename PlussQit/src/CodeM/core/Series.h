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
#include "pluss/subsp/linearalg/linearalg_fwd.h"
#include "pluss/subsp/stats/stats_fwd.h"
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

  // Statistical namespace-like proxy.
  // Supports both:
  //   series.stat().mean()
  //   series.stat.mean()
  // and peer-bound usage:
  //   series1.stat(series2).pdist2()
  //   series1.stat({series2, series3}).call("some_op", ...)
  class SeriesStatProxy final {
  public:
    explicit SeriesStatProxy(const Series& self) : self_(&self) {}
    SeriesStatProxy(const Series& self, const Series& other) : self_(&self), peers_{other} {}
    SeriesStatProxy(const Series& self, std::initializer_list<Series> others)
        : self_(&self), peers_(others.begin(), others.end()) {}
    SeriesStatProxy(const Series& self, std::span<const Series> others)
        : self_(&self), peers_(others.begin(), others.end()) {}

    // Central tendency / spread
    Series mean() const { return pluss::subsp::stats::mean(self()); }
    Series mean(std::int64_t dim, bool keepdim = false) const {
      return pluss::subsp::stats::mean(self(), dim, keepdim);
    }
    Series var(bool unbiased = true) const { return pluss::subsp::stats::var(self(), unbiased); }
    Series var(std::int64_t dim, bool unbiased = true, bool keepdim = false) const {
      return pluss::subsp::stats::var(self(), dim, unbiased, keepdim);
    }
    Series std(bool unbiased = true) const { return pluss::subsp::stats::std(self(), unbiased); }
    Series std(std::int64_t dim, bool unbiased = true, bool keepdim = false) const {
      return pluss::subsp::stats::std(self(), dim, unbiased, keepdim);
    }
    Series nanvar() const { return pluss::subsp::stats::nanvar(self()); }
    Series nanstd() const { return pluss::subsp::stats::nanstd(self()); }
    Series skewness() const { return pluss::subsp::stats::skewness(self()); }
    Series kurtosis() const { return pluss::subsp::stats::kurtosis(self()); }

    // Rolling stats
    template <class... Args>
    Series movsum(Args&&... args) const { return invoke_self_("movsum", std::forward<Args>(args)...); }
    template <class... Args>
    Series movmean(Args&&... args) const { return invoke_self_("movmean", std::forward<Args>(args)...); }
    template <class... Args>
    Series movmax(Args&&... args) const { return invoke_self_("movmax", std::forward<Args>(args)...); }
    template <class... Args>
    Series movmin(Args&&... args) const { return invoke_self_("movmin", std::forward<Args>(args)...); }
    template <class... Args>
    Series movstd(Args&&... args) const { return invoke_self_("movstd", std::forward<Args>(args)...); }
    template <class... Args>
    Series movvar(Args&&... args) const { return invoke_self_("movvar", std::forward<Args>(args)...); }

    // Normalization-style stats
    Series normr() const { return pluss::subsp::stats::normr(self()); }
    Series normc() const { return pluss::subsp::stats::normc(self()); }
    Series standardize() const { return pluss::subsp::stats::standardize(self()); }
    Series center() const { return pluss::subsp::stats::center(self()); }

    // Pairwise stats (require one peer Series either bound via stat(peer) or passed directly).
    Series pdist2() const { return pluss::subsp::stats::pdist2(self(), peer1()); }
    Series pdist2(const Series& other) const { return pluss::subsp::stats::pdist2(self(), other); }
    Series cdist() const { return pluss::subsp::stats::cdist(self(), peer1()); }
    Series cdist(const Series& other) const { return pluss::subsp::stats::cdist(self(), other); }

    // Escape hatch for quickly trying stat-family ops not yet sugar-wrapped here.
    template <class... Args>
    Series call(std::string_view op, Args&&... args) const {
      return invoke_self_(op, std::forward<Args>(args)...);
    }

  private:
    const Series& self() const {
      PLUSS_CHECK(self_ != nullptr, "Series.stat(): null self");
      return *self_;
    }

    const Series& peer1() const {
      PLUSS_CHECK(!peers_.empty(), "Series.stat(...): this operation requires one peer Series");
      return peers_.front();
    }

    template <class... Args>
    Series invoke_self_(std::string_view op, Args&&... args) const {
      return _compat::invoke(op, self(), std::forward<Args>(args)...);
    }

    template <class... Args>
    Series invoke_peer1_(std::string_view op, Args&&... args) const {
      return _compat::invoke(op, self(), peer1(), std::forward<Args>(args)...);
    }

    const Series* self_{nullptr};
    std::vector<Series> peers_{};
  };

  class SeriesStatAccessor final {
  public:
    SeriesStatAccessor() = default;
    explicit SeriesStatAccessor(Series& owner) : owner_(&owner) {}

    void rebind(Series& owner) noexcept { owner_ = &owner; }

    SeriesStatProxy operator()() const { return SeriesStatProxy(owner()); }
    SeriesStatProxy operator()(const Series& other) const { return SeriesStatProxy(owner(), other); }
    SeriesStatProxy operator()(std::initializer_list<Series> others) const {
      return SeriesStatProxy(owner(), others);
    }
    SeriesStatProxy operator()(std::span<const Series> others) const {
      return SeriesStatProxy(owner(), others);
    }

    // Dot-style convenience for no-arg stat operations.
    Series mean() const { return (*this)().mean(); }
    Series mean(std::int64_t dim, bool keepdim = false) const { return (*this)().mean(dim, keepdim); }
    Series var(bool unbiased = true) const { return (*this)().var(unbiased); }
    Series var(std::int64_t dim, bool unbiased = true, bool keepdim = false) const {
      return (*this)().var(dim, unbiased, keepdim);
    }
    Series std(bool unbiased = true) const { return (*this)().std(unbiased); }
    Series std(std::int64_t dim, bool unbiased = true, bool keepdim = false) const {
      return (*this)().std(dim, unbiased, keepdim);
    }
    Series nanvar() const { return (*this)().nanvar(); }
    Series nanstd() const { return (*this)().nanstd(); }
    Series skewness() const { return (*this)().skewness(); }
    Series kurtosis() const { return (*this)().kurtosis(); }
    template <class... Args>
    Series movsum(Args&&... args) const { return (*this)().movsum(std::forward<Args>(args)...); }
    template <class... Args>
    Series movmean(Args&&... args) const { return (*this)().movmean(std::forward<Args>(args)...); }
    template <class... Args>
    Series movmax(Args&&... args) const { return (*this)().movmax(std::forward<Args>(args)...); }
    template <class... Args>
    Series movmin(Args&&... args) const { return (*this)().movmin(std::forward<Args>(args)...); }
    template <class... Args>
    Series movstd(Args&&... args) const { return (*this)().movstd(std::forward<Args>(args)...); }
    template <class... Args>
    Series movvar(Args&&... args) const { return (*this)().movvar(std::forward<Args>(args)...); }
    Series normr() const { return (*this)().normr(); }
    Series normc() const { return (*this)().normc(); }
    Series standardize() const { return (*this)().standardize(); }
    Series center() const { return (*this)().center(); }
    Series pdist2() const { return (*this)().pdist2(); }
    Series pdist2(const Series& other) const { return (*this)().pdist2(other); }
    Series cdist() const { return (*this)().cdist(); }
    Series cdist(const Series& other) const { return (*this)().cdist(other); }
    template <class... Args>
    Series call(std::string_view op, Args&&... args) const {
      return (*this)().call(op, std::forward<Args>(args)...);
    }

  private:
    Series& owner() const {
      PLUSS_CHECK(owner_ != nullptr, "Series.stat: unbound accessor");
      return *owner_;
    }

    Series* owner_{nullptr};
  };

  // Linear algebra namespace-like proxy.
  // Supports both:
  //   series.linearalg().det()
  //   series.linearalg.det()
  // and peer-bound usage:
  //   series1.linearalg(series2).matmul()
  class SeriesLinearAlgProxy final {
  public:
    explicit SeriesLinearAlgProxy(const Series& self) : self_(&self) {}
    SeriesLinearAlgProxy(const Series& self, const Series& other) : self_(&self), peers_{other} {}
    SeriesLinearAlgProxy(const Series& self, std::initializer_list<Series> others)
        : self_(&self), peers_(others.begin(), others.end()) {}
    SeriesLinearAlgProxy(const Series& self, std::span<const Series> others)
        : self_(&self), peers_(others.begin(), others.end()) {}

    Series matmul() const { return pluss::subsp::linearalg::matmul(self(), peer1()); }
    Series matmul(const Series& other) const { return pluss::subsp::linearalg::matmul(self(), other); }
    Series mm() const { return pluss::subsp::linearalg::mm(self(), peer1()); }
    Series mm(const Series& other) const { return pluss::subsp::linearalg::mm(self(), other); }
    Series mv() const { return pluss::subsp::linearalg::mv(self(), peer1()); }
    Series mv(const Series& other) const { return pluss::subsp::linearalg::mv(self(), other); }
    Series dot() const { return pluss::subsp::linearalg::dot(self(), peer1()); }
    Series dot(const Series& other) const { return pluss::subsp::linearalg::dot(self(), other); }
    Series vdot() const { return pluss::subsp::linearalg::vdot(self(), peer1()); }
    Series vdot(const Series& other) const { return pluss::subsp::linearalg::vdot(self(), other); }
    Series norm() const { return pluss::subsp::linearalg::norm(self()); }

    Series det() const { return pluss::subsp::linearalg::det(self()); }
    Series inverse() const { return pluss::subsp::linearalg::inverse(self()); }
    Series cholesky(bool upper = false) const { return pluss::subsp::linearalg::cholesky(self(), upper); }
    Series logdet() const { return pluss::subsp::linearalg::logdet(self()); }
    Series slogdet() const { return pluss::subsp::linearalg::slogdet(self()); }
    Series matrix_power(std::int64_t n) const { return pluss::subsp::linearalg::matrix_power(self(), n); }
    Series matrix_exp() const { return pluss::subsp::linearalg::matrix_exp(self()); }
    Series pinverse(double rcond = 1e-12) const { return pluss::subsp::linearalg::pinverse(self(), rcond); }
    Series qr_q(bool reduced = true) const { return pluss::subsp::linearalg::qr_q(self(), reduced); }
    Series qr_r(bool reduced = true) const { return pluss::subsp::linearalg::qr_r(self(), reduced); }
    Series svd_u(bool full_matrices = true) const { return pluss::subsp::linearalg::svd_u(self(), full_matrices); }
    Series svd_s(bool full_matrices = true) const { return pluss::subsp::linearalg::svd_s(self(), full_matrices); }
    Series svd_vh(bool full_matrices = true) const { return pluss::subsp::linearalg::svd_vh(self(), full_matrices); }

    Series triangular_solve(const Series& b, bool upper = true, bool unitriangular = false) const {
      return pluss::subsp::linearalg::triangular_solve(self(), b, upper, unitriangular);
    }
    Series triangular_solve(bool upper = true, bool unitriangular = false) const {
      return pluss::subsp::linearalg::triangular_solve(self(), peer1(), upper, unitriangular);
    }
    Series cholesky_solve(const Series& b, bool upper = false) const {
      return pluss::subsp::linearalg::cholesky_solve(self(), b, upper);
    }
    Series cholesky_solve(bool upper = false) const {
      return pluss::subsp::linearalg::cholesky_solve(self(), peer1(), upper);
    }
    Series cholesky_inverse(bool upper = false) const {
      return pluss::subsp::linearalg::cholesky_inverse(self(), upper);
    }
    Series lu_solve(const Series& b) const { return pluss::subsp::linearalg::lu_solve(self(), b); }
    Series lu_solve() const { return pluss::subsp::linearalg::lu_solve(self(), peer1()); }

    template <class... Args>
    Series call(std::string_view op, Args&&... args) const {
      return _compat::invoke(op, self(), std::forward<Args>(args)...);
    }

  private:
    const Series& self() const {
      PLUSS_CHECK(self_ != nullptr, "Series.linearalg(): null self");
      return *self_;
    }

    const Series& peer1() const {
      PLUSS_CHECK(!peers_.empty(), "Series.linearalg(...): this operation requires one peer Series");
      return peers_.front();
    }

    const Series* self_{nullptr};
    std::vector<Series> peers_{};
  };

  class SeriesLinearAlgAccessor final {
  public:
    SeriesLinearAlgAccessor() = default;
    explicit SeriesLinearAlgAccessor(Series& owner) : owner_(&owner) {}

    void rebind(Series& owner) noexcept { owner_ = &owner; }

    SeriesLinearAlgProxy operator()() const { return SeriesLinearAlgProxy(owner()); }
    SeriesLinearAlgProxy operator()(const Series& other) const { return SeriesLinearAlgProxy(owner(), other); }
    SeriesLinearAlgProxy operator()(std::initializer_list<Series> others) const {
      return SeriesLinearAlgProxy(owner(), others);
    }
    SeriesLinearAlgProxy operator()(std::span<const Series> others) const {
      return SeriesLinearAlgProxy(owner(), others);
    }

    Series matmul(const Series& other) const { return (*this)().matmul(other); }
    Series mm(const Series& other) const { return (*this)().mm(other); }
    Series mv(const Series& other) const { return (*this)().mv(other); }
    Series dot(const Series& other) const { return (*this)().dot(other); }
    Series vdot(const Series& other) const { return (*this)().vdot(other); }
    Series norm() const { return (*this)().norm(); }
    Series det() const { return (*this)().det(); }
    Series inverse() const { return (*this)().inverse(); }
    Series cholesky(bool upper = false) const { return (*this)().cholesky(upper); }
    Series logdet() const { return (*this)().logdet(); }
    Series slogdet() const { return (*this)().slogdet(); }
    Series matrix_power(std::int64_t n) const { return (*this)().matrix_power(n); }
    Series matrix_exp() const { return (*this)().matrix_exp(); }
    Series pinverse(double rcond = 1e-12) const { return (*this)().pinverse(rcond); }
    Series qr_q(bool reduced = true) const { return (*this)().qr_q(reduced); }
    Series qr_r(bool reduced = true) const { return (*this)().qr_r(reduced); }
    Series svd_u(bool full_matrices = true) const { return (*this)().svd_u(full_matrices); }
    Series svd_s(bool full_matrices = true) const { return (*this)().svd_s(full_matrices); }
    Series svd_vh(bool full_matrices = true) const { return (*this)().svd_vh(full_matrices); }
    Series triangular_solve(const Series& b, bool upper = true, bool unitriangular = false) const {
      return (*this)().triangular_solve(b, upper, unitriangular);
    }
    Series cholesky_solve(const Series& b, bool upper = false) const {
      return (*this)().cholesky_solve(b, upper);
    }
    Series cholesky_inverse(bool upper = false) const { return (*this)().cholesky_inverse(upper); }
    Series lu_solve(const Series& b) const { return (*this)().lu_solve(b); }
    template <class... Args>
    Series call(std::string_view op, Args&&... args) const {
      return (*this)().call(op, std::forward<Args>(args)...);
    }

  private:
    Series& owner() const {
      PLUSS_CHECK(owner_ != nullptr, "Series.linearalg: unbound accessor");
      return *owner_;
    }

    Series* owner_{nullptr};
  };

  // Factory helpers (Torch-style free-functions live elsewhere; keep these for convenience).
  static Series undefined();
  static Series empty(std::span<const std::int64_t> sizes, ScalarType st);
  static Series empty(std::span<const std::int64_t> sizes, ScalarType st, Platform platform);
  static Series empty(std::initializer_list<std::int64_t> sizes, ScalarType st) {
    return empty(std::span<const std::int64_t>(sizes.begin(), sizes.size()), st);
  }
  static Series empty(std::initializer_list<std::int64_t> sizes, ScalarType st, Platform platform) {
    return empty(std::span<const std::int64_t>(sizes.begin(), sizes.size()), st, platform);
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
  // Torch-like transfer/cast entrypoints.
  // Non-CPU backend support is incremental (MPS currently supports a subset).
  Series to(Platform target) const;
  Series to(Platform target, ScalarType st) const;
  Series clone() const;
  // Query current backend (Torch-like tensor.device() analog at Platform granularity).
  Platform platform() const noexcept { return SeriesBase::platform(); }
  // Request transfer to another backend.
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

  // Scalar extraction (Torch-style): requires exactly one element.
  // Returns a native C++ value of the requested type.
  template <class T>
  T item() const {
    PLUSS_CHECK(numel() == 1, "Series::item<T>: expected tensor with exactly one element");
    return to_vec<T>(0);
  }
  // Dynamic scalar extraction: requires exactly one element.
  // Returns a tagged Scalar when caller doesn't want to spell an explicit C++ type.
  // Use item<T>() when exact native type extraction is required.
  Scalar item() const {
    PLUSS_CHECK(numel() == 1, "Series::item(): expected tensor with exactly one element");
    switch (scalar_type()) {
      case ScalarType::Bool:
        return Scalar(to_vec<bool>(0), ScalarType::Bool);
      case ScalarType::UInt8:
        return Scalar(static_cast<std::uint64_t>(to_vec<std::uint8_t>(0)), ScalarType::UInt8);
      case ScalarType::UInt16:
        return Scalar(static_cast<std::uint64_t>(to_vec<std::uint16_t>(0)), ScalarType::UInt16);
      case ScalarType::UInt32:
        return Scalar(static_cast<std::uint64_t>(to_vec<std::uint32_t>(0)), ScalarType::UInt32);
      case ScalarType::UInt64:
        return Scalar(to_vec<std::uint64_t>(0), ScalarType::UInt64);
      case ScalarType::Int8:
        return Scalar(static_cast<std::int64_t>(to_vec<std::int8_t>(0)), ScalarType::Int8);
      case ScalarType::Int16:
        return Scalar(static_cast<std::int64_t>(to_vec<std::int16_t>(0)), ScalarType::Int16);
      case ScalarType::Int32:
        return Scalar(static_cast<std::int64_t>(to_vec<std::int32_t>(0)), ScalarType::Int32);
      case ScalarType::Int64:
        return Scalar(to_vec<std::int64_t>(0), ScalarType::Int64);
      case ScalarType::Float16:
      case ScalarType::BFloat16:
      case ScalarType::Float8E4M3FN:
      case ScalarType::Float8E5M2:
        return Scalar(to(ScalarType::Float64).to_vec<double>(0), scalar_type());
      case ScalarType::Float32:
        return Scalar(static_cast<double>(to_vec<float>(0)), ScalarType::Float32);
      case ScalarType::Float64:
        return Scalar(to_vec<double>(0), ScalarType::Float64);
      case ScalarType::Complex64:
      case ScalarType::Complex128:
        PLUSS_FAIL("Series::item(): complex dtype requires explicit item<Complex64>() or item<Complex128>()");
      default:
        PLUSS_FAIL("Series::item(): unsupported dtype for dynamic scalar extraction");
    }
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

  Series reshape(std::initializer_list<std::int64_t> new_sizes) const {
    return reshape(std::span<const std::int64_t>(new_sizes.begin(), new_sizes.size()));
  }


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

  // Pluss autodiff entrypoint. This is a named wrapper over the existing
  // generated `_backward(...)` method so public docs can standardize on
  // `reverse()` without forking the engine path.
  void reverse() const { (void)pluss::codem::_compat::invoke("_backward", *this); }
  void reverse(const Series& sensitivity_output) const {
    (void)pluss::codem::_compat::invoke("_backward", *this, sensitivity_output);
  }
  bool tracks_slope() const { return pluss::codem::_compat::invoke("tracks_slope", *this).item<bool>(); }
  bool has_sensitivity() const {
    return pluss::codem::_compat::invoke("has_sensitivity", *this).item<bool>();
  }
  Series sensitivity() const { return pluss::codem::_compat::invoke("sensitivity", *this); }
  void clear_sensitivity() const { (void)pluss::codem::_compat::invoke_("clear_sensitivity", *this); }
  std::int64_t storage_version() const {
    return pluss::codem::_compat::invoke("_version", *this).item<std::int64_t>();
  }

  // PlussQit naming deviations (method-first aliases).
  Series pile(const std::vector<Series>& others, std::int64_t dim=0) const;
  Series unite(const std::vector<Series>& others, std::int64_t dim=0) const;
  Series index(std::initializer_list<Series> indices) const {
    return index(std::vector<Series>(indices.begin(), indices.end()));
  }
  Series index_put(const std::vector<Series>& indices, const Series& values, bool accumulate) const {
    return pluss::codem::_compat::invoke("index_put", *this, indices, values, accumulate);
  }
  Series index_put(std::initializer_list<Series> indices, const Series& values, bool accumulate) const {
    return index_put(std::vector<Series>(indices.begin(), indices.end()), values, accumulate);
  }
  Series& index_put_(const std::vector<Series>& indices, const Series& values, bool accumulate) const {
    return pluss::codem::_compat::invoke_("index_put_", *this, indices, values, accumulate);
  }
  Series& index_put_(std::initializer_list<Series> indices, const Series& values, bool accumulate) const {
    return index_put_(std::vector<Series>(indices.begin(), indices.end()), values, accumulate);
  }

  // Statistical accessor:
  // - safe call style: `s.stat().mean()`
  // - convenience dot style: `s.stat.mean()`
  SeriesStatAccessor stat;
  // Linear algebra accessor:
  // - safe call style: `s.linearalg().det()`
  // - convenience dot style: `s.linearalg.det()`
  SeriesLinearAlgAccessor linearalg;

  Series() : SeriesBase(), stat(*this), linearalg(*this) {}
  Series(const Series& other) : SeriesBase(other), stat(*this), linearalg(*this) {}
  Series(Series&& other) noexcept : SeriesBase(std::move(other)), stat(*this), linearalg(*this) {}
  Series& operator=(const Series& other) {
    if (this != &other) {
      SeriesBase::operator=(other);
    }
    stat.rebind(*this);
    linearalg.rebind(*this);
    return *this;
  }
  Series& operator=(Series&& other) noexcept {
    if (this != &other) {
      SeriesBase::operator=(std::move(other));
    }
    stat.rebind(*this);
    linearalg.rebind(*this);
    return *this;
  }

private:
  template <class>
  struct detail_false_ : std::false_type {};

  template <class T>
  std::vector<T> detail_materialize_vec_() const {
    if (platform() != Platform::CPU) {
      return to(Platform::CPU).template detail_materialize_vec_<T>();
    }

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

  // Internal-only ctor: Series objects are created via factories and internal
  // kernels (ops_internal), not by end users.
  explicit Series(pluss::msharp::intrusive_ptr<SeriesImpl> impl)
      : SeriesBase(std::move(impl)), stat(*this), linearalg(*this) {}

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
