#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <span>
#include <sstream>
#include <string>
#include <variant>
#include <vector>
#include <type_traits>
#include <iosfwd>
#include <utility>
#include <limits>


#include "CodeM/core/Errors.h"
#include "CodeM/Indexing/ExtractSpec.h"
#include "CodeM/Traverse/Traverse.h"

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
#include "pluss/slicing.h"
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

namespace pluss {
template <class T>
class StackArray;
}

namespace pluss::codem {

namespace ops_internal {
struct SeriesFactoryAccess;
} // namespace ops_internal

struct TernaryCompareBuilder;


// Forward declaration for tensor memory/layout format used by generated APIs.
// Full definition should come from the appropriate header if/when needed.
enum class MemoryFormat : std::uint8_t;

// User-facing Series (Torch-like Tensor).
class CODEM_API Series : public SeriesBase {
public:
  using SeriesBase::size;
  std::int64_t size(int dim) const { return SeriesBase::size(static_cast<std::int64_t>(dim)); }
  std::int64_t size(std::int64_t dim) const { return SeriesBase::size(dim); }

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

  using HostScalarAny = std::variant<
      bool,
      std::uint8_t,
      std::uint16_t,
      std::uint32_t,
      std::uint64_t,
      std::int8_t,
      std::int16_t,
      std::int32_t,
      std::int64_t,
      Float16,
      BFloat16,
      float,
      double,
      Complex64,
      Complex128>;

  using HostVectorAny = std::variant<
      std::vector<bool>,
      std::vector<std::uint8_t>,
      std::vector<std::uint16_t>,
      std::vector<std::uint32_t>,
      std::vector<std::uint64_t>,
      std::vector<std::int8_t>,
      std::vector<std::int16_t>,
      std::vector<std::int32_t>,
      std::vector<std::int64_t>,
      std::vector<Float16>,
      std::vector<BFloat16>,
      std::vector<float>,
      std::vector<double>,
      std::vector<Complex64>,
      std::vector<Complex128>>;

  struct HostArrayAny final {
    HostVectorAny data;
    std::vector<std::int64_t> sizes;
    std::vector<std::int64_t> strides;
    bool contiguous{false};

    std::span<const std::int64_t> shape() const noexcept { return sizes; }
    std::span<const std::int64_t> stride_view() const noexcept { return strides; }
    std::int64_t dim() const noexcept { return static_cast<std::int64_t>(sizes.size()); }
    std::int64_t numel() const noexcept {
      return std::visit(
          [](const auto& v) -> std::int64_t {
            return static_cast<std::int64_t>(v.size());
          },
          data);
    }

    template <class T>
    std::span<const T> flat_span_as() const {
      const auto& v = std::get<std::vector<T>>(data);
      if constexpr (std::is_same_v<T, bool>) {
        PLUSS_FAIL("Series::host(): bool payload does not expose span view; use vector_as<bool>()");
      } else {
        return std::span<const T>(v.data(), v.size());
      }
    }
  };

  class HostAny final {
  public:
    enum class Kind : std::uint8_t { Scalar = 0, Vector = 1, Array = 2 };

    HostAny() = default;

    static HostAny from_scalar(HostScalarAny scalar, ScalarType dtype) {
      HostAny out;
      out.kind_ = Kind::Scalar;
      out.dtype_ = dtype;
      out.payload_ = std::move(scalar);
      return out;
    }

    static HostAny from_vector(HostVectorAny vec, ScalarType dtype) {
      HostAny out;
      out.kind_ = Kind::Vector;
      out.dtype_ = dtype;
      out.payload_ = std::move(vec);
      return out;
    }

    static HostAny from_array(HostArrayAny arr, ScalarType dtype) {
      HostAny out;
      out.kind_ = Kind::Array;
      out.dtype_ = dtype;
      out.payload_ = std::move(arr);
      return out;
    }

    Kind kind() const noexcept { return kind_; }
    ScalarType dtype() const noexcept { return dtype_; }
    bool is_scalar() const noexcept { return kind_ == Kind::Scalar; }
    bool is_vector() const noexcept { return kind_ == Kind::Vector; }
    bool is_array() const noexcept { return kind_ == Kind::Array; }

    const HostScalarAny& scalar() const {
      PLUSS_CHECK(is_scalar(), "Series::host(): value is not scalar");
      return std::get<HostScalarAny>(payload_);
    }

    const HostVectorAny& vector() const {
      PLUSS_CHECK(is_vector(), "Series::host(): value is not a 1-D vector");
      return std::get<HostVectorAny>(payload_);
    }

    const HostArrayAny& array() const {
      PLUSS_CHECK(is_array(), "Series::host(): value is not N-D array");
      return std::get<HostArrayAny>(payload_);
    }

    template <class T>
    const T& scalar_as() const {
      return std::get<T>(scalar());
    }

    template <class T>
    const std::vector<T>& vector_as() const {
      return std::get<std::vector<T>>(vector());
    }

    template <class T>
    std::span<const T> vector_span_as() const {
      const auto& v = vector_as<T>();
      if constexpr (std::is_same_v<T, bool>) {
        PLUSS_FAIL("Series::host(): bool payload does not expose span view; use vector_as<bool>()");
      } else {
        return std::span<const T>(v.data(), v.size());
      }
    }

    template <class T>
    std::span<const T> array_flat_span_as() const {
      return array().template flat_span_as<T>();
    }

    template <class T>
    T to_scalar() const {
      PLUSS_CHECK(is_scalar(), "Series::host(): scalar conversion requested on non-scalar");
      return std::visit(
          [](const auto& v) -> T {
            using U = std::remove_cv_t<std::remove_reference_t<decltype(v)>>;
            if constexpr (std::is_same_v<U, Float16>) {
              return static_cast<T>(float16_to_float(v));
            } else if constexpr (std::is_same_v<U, BFloat16>) {
              return static_cast<T>(bfloat16_to_float(v));
            } else {
              return static_cast<T>(v);
            }
          },
          scalar());
    }

    double to_double() const {
      PLUSS_CHECK(is_scalar(), "Series::host(): scalar numeric conversion requested on non-scalar");
      return std::visit(
          [](const auto& v) -> double {
            using U = std::remove_cv_t<std::remove_reference_t<decltype(v)>>;
            if constexpr (std::is_same_v<U, bool>) {
              return v ? 1.0 : 0.0;
            } else if constexpr (std::is_same_v<U, Float16>) {
              return static_cast<double>(float16_to_float(v));
            } else if constexpr (std::is_same_v<U, BFloat16>) {
              return static_cast<double>(bfloat16_to_float(v));
            } else if constexpr (std::is_same_v<U, Complex64> || std::is_same_v<U, Complex128>) {
              PLUSS_CHECK(std::abs(v.imag()) == 0.0, "Series::host(): cannot cast complex scalar with non-zero imaginary part to double");
              return static_cast<double>(v.real());
            } else {
              return static_cast<double>(v);
            }
          },
          scalar());
    }

    operator double() const { return to_double(); }

    friend std::ostream& operator<<(std::ostream& os, Kind k) {
      switch (k) {
        case Kind::Scalar: os << "Scalar"; break;
        case Kind::Vector: os << "Vector"; break;
        case Kind::Array: os << "Array"; break;
      }
      return os;
    }

    friend std::ostream& operator<<(std::ostream& os, const HostAny& v) {
      auto append_scalar = [&](const auto& x) {
        using U = std::remove_cv_t<std::remove_reference_t<decltype(x)>>;
        if constexpr (std::is_same_v<U, bool>) {
          os << (x ? "true" : "false");
        } else if constexpr (std::is_same_v<U, Float16>) {
          os << float16_to_float(x);
        } else if constexpr (std::is_same_v<U, BFloat16>) {
          os << bfloat16_to_float(x);
        } else {
          os << x;
        }
      };

      auto append_vector = [&](const auto& vec) {
        os << "[";
        for (std::size_t i = 0; i < vec.size(); ++i) {
          if (i) os << ", ";
          append_scalar(vec[i]);
        }
        os << "]";
      };

      if (v.is_scalar()) {
        std::visit([&](const auto& x) { append_scalar(x); }, v.scalar());
        return os;
      }
      if (v.is_vector()) {
        std::visit([&](const auto& vec) { append_vector(vec); }, v.vector());
        return os;
      }
      const auto& arr = v.array();
      os << "{shape=[";
      for (std::size_t i = 0; i < arr.sizes.size(); ++i) {
        if (i) os << ",";
        os << arr.sizes[i];
      }
      os << "], flat=";
      std::visit([&](const auto& vec) { append_vector(vec); }, arr.data);
      os << "}";
      return os;
    }

  private:
    Kind kind_{Kind::Scalar};
    ScalarType dtype_{ScalarType::Float64};
    std::variant<HostScalarAny, HostVectorAny, HostArrayAny> payload_{HostScalarAny{0.0}};
  };

  // Dynamic host export wrapper:
  // - scalar Series -> scalar T
  // - 1-D Series    -> std::vector<T>
  // - N-D Series    -> HostArray<T>
  // `array()` always preserves exact shape/stride/contiguous metadata from
  // the source Series (including scalar/1-D cases).
  template <class T>
  class HostValue {
  public:
    enum class Kind : std::uint8_t { Scalar = 0, Vector = 1, Array = 2 };

    HostValue() = default;
    explicit HostValue(HostArray<T> arr, Kind kind = Kind::Array)
        : arr_(std::move(arr)), kind_(kind) {}

    static HostValue from_scalar(HostArray<T> arr) { return HostValue(std::move(arr), Kind::Scalar); }
    static HostValue from_vector(HostArray<T> arr) { return HostValue(std::move(arr), Kind::Vector); }
    static HostValue from_array(HostArray<T> arr) { return HostValue(std::move(arr), Kind::Array); }

    Kind kind() const noexcept { return kind_; }
    bool is_scalar() const noexcept { return kind_ == Kind::Scalar; }
    bool is_vector() const noexcept { return kind_ == Kind::Vector; }
    bool is_array() const noexcept { return kind_ == Kind::Array; }

    const T& scalar() const {
      PLUSS_CHECK(is_scalar(), "Series::HostValue: not scalar");
      PLUSS_CHECK(arr_.data.size() == 1, "Series::HostValue: scalar payload must contain exactly one element");
      return arr_.data.front();
    }

    std::vector<T> vector() const {
      if (is_scalar()) return std::vector<T>{scalar()};
      return arr_.data;
    }

    const HostArray<T>& array() const { return arr_; }

    operator T() const { return scalar(); }
    operator std::vector<T>() const { return vector(); }
    operator HostArray<T>() const { return arr_; }

    friend std::ostream& operator<<(std::ostream& os, const HostValue& v) {
      auto append_scalar = [&](const auto& x) {
        using U = std::remove_cv_t<std::remove_reference_t<decltype(x)>>;
        if constexpr (std::is_same_v<U, bool>) {
          os << (x ? "true" : "false");
        } else if constexpr (std::is_same_v<U, Float16>) {
          os << float16_to_float(x);
        } else if constexpr (std::is_same_v<U, BFloat16>) {
          os << bfloat16_to_float(x);
        } else {
          os << x;
        }
      };

      auto append_vector = [&](const std::vector<T>& vec) {
        os << "[";
        for (std::size_t i = 0; i < vec.size(); ++i) {
          if (i) os << ", ";
          append_scalar(vec[i]);
        }
        os << "]";
      };

      if (v.is_scalar()) {
        append_scalar(v.scalar());
        return os;
      }
      if (v.is_vector()) {
        append_vector(v.vector());
        return os;
      }
      const auto& arr = v.array();
      os << "{shape=[";
      for (std::size_t i = 0; i < arr.sizes.size(); ++i) {
        if (i) os << ",";
        os << arr.sizes[i];
      }
      os << "], flat=";
      append_vector(arr.data);
      os << "}";
      return os;
    }

  private:
    HostArray<T> arr_{};
    Kind kind_{Kind::Array};
  };

  struct Attr final {
    std::vector<std::int64_t> shape;
    std::vector<std::int64_t> strides;
    ScalarType dtype{ScalarType::Float64};
    Platform platform{Platform::CPU};
    std::int64_t numel{0};
    bool contiguous{false};
    HostAny values{};

    [[nodiscard]] std::span<const std::int64_t> shape_view() const noexcept { return shape; }
    [[nodiscard]] std::span<const std::int64_t> stride_view() const noexcept { return strides; }
    [[nodiscard]] std::int64_t rank() const noexcept { return static_cast<std::int64_t>(shape.size()); }
    [[nodiscard]] ScalarType scalar_type() const noexcept { return dtype; }
    [[nodiscard]] Platform device() const noexcept { return platform; }
    [[nodiscard]] bool is_contiguous() const noexcept { return contiguous; }
    [[nodiscard]] bool is_scalar() const noexcept { return numel == 1; }
    [[nodiscard]] bool is_vector() const noexcept { return rank() <= 1 && numel > 1; }
    [[nodiscard]] bool is_array() const noexcept { return rank() > 1; }
    [[nodiscard]] HostAny::Kind value_kind() const noexcept { return values.kind(); }

    [[nodiscard]] std::string to_string(std::int64_t edge_items = 3) const {
      std::ostringstream oss;
      oss << "SeriesAttr(";
      oss << "shape=" << dims_to_string_(shape);
      oss << ", strides=" << dims_to_string_(strides);
      oss << ", dtype=" << dtype;
      oss << ", platform=" << platform;
      oss << ", numel=" << numel;
      oss << ", contiguous=" << (contiguous ? "true" : "false");
      oss << ", values=" << host_any_to_string_(values, edge_items);
      oss << ")";
      return oss.str();
    }

    friend std::ostream& operator<<(std::ostream& os, const Attr& a) {
      os << a.to_string();
      return os;
    }

  private:
    static std::string dims_to_string_(const std::vector<std::int64_t>& xs) {
      std::ostringstream oss;
      oss << "[";
      for (std::size_t i = 0; i < xs.size(); ++i) {
        if (i) oss << ",";
        oss << xs[i];
      }
      oss << "]";
      return oss.str();
    }

    template <class U>
    static void append_scalar_(std::ostringstream& oss, const U& v) {
      using V = std::remove_cv_t<std::remove_reference_t<U>>;
      if constexpr (std::is_same_v<V, bool>) {
        oss << (v ? "true" : "false");
      } else if constexpr (std::is_same_v<V, Float16>) {
        oss << float16_to_float(v);
      } else if constexpr (std::is_same_v<V, BFloat16>) {
        oss << bfloat16_to_float(v);
      } else {
        oss << v;
      }
    }

    template <class VecT>
    static void append_vector_preview_(std::ostringstream& oss, const VecT& v, std::int64_t edge_items) {
      const std::size_t n = v.size();
      const std::size_t k = static_cast<std::size_t>(std::max<std::int64_t>(1, edge_items));
      oss << "[";
      if (n <= 2 * k) {
        for (std::size_t i = 0; i < n; ++i) {
          if (i) oss << ", ";
          append_scalar_(oss, v[i]);
        }
      } else {
        for (std::size_t i = 0; i < k; ++i) {
          if (i) oss << ", ";
          append_scalar_(oss, v[i]);
        }
        oss << ", ..., ";
        for (std::size_t i = n - k; i < n; ++i) {
          if (i > n - k) oss << ", ";
          append_scalar_(oss, v[i]);
        }
      }
      oss << "]";
    }

    static std::string host_any_to_string_(const HostAny& any, std::int64_t edge_items) {
      std::ostringstream oss;
      if (any.is_scalar()) {
        std::visit([&](const auto& x) { append_scalar_(oss, x); }, any.scalar());
        return oss.str();
      }
      if (any.is_vector()) {
        std::visit([&](const auto& v) { append_vector_preview_(oss, v, edge_items); }, any.vector());
        return oss.str();
      }
      const auto& arr = any.array();
      oss << "{shape=" << dims_to_string_(arr.sizes) << ", flat=";
      std::visit([&](const auto& v) { append_vector_preview_(oss, v, edge_items); }, arr.data);
      oss << "}";
      return oss.str();
    }
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

    // Distribution helpers (kept on stat surface; not on core Series surface).
    Series pdf() const { return _compat::invoke("pdf", self()); }
    Series pdf(double sigma) const { return _compat::invoke("pdf", self(), {sigma}); }
    Series pdf(double mu, double sigma) const { return _compat::invoke("pdf", self(), {mu, sigma}); }

    Series logpdf() const { return _compat::invoke("logpdf", self()); }
    Series logpdf(double sigma) const { return _compat::invoke("logpdf", self(), {sigma}); }
    Series logpdf(double mu, double sigma) const { return _compat::invoke("logpdf", self(), {mu, sigma}); }

    Series logcdf() const { return _compat::invoke("logcdf", self()); }
    Series logcdf(double sigma) const { return _compat::invoke("logcdf", self(), {sigma}); }
    Series logcdf(double mu, double sigma) const { return _compat::invoke("logcdf", self(), {mu, sigma}); }

    Series cdf() const { return _compat::invoke("cdf", self()); }
    Series cdf(double sigma) const { return _compat::invoke("cdf", self(), {sigma}); }
    Series cdf(double mu, double sigma) const { return _compat::invoke("cdf", self(), {mu, sigma}); }

    // Rolling stats
    Series movsum() const { return _compat::invoke("movsum", self()); }
    Series movsum(std::int64_t window) const { return _compat::invoke("movsum", self(), {window}); }
    Series movsum(std::int64_t window, std::int64_t dim) const {
      return _compat::invoke("movsum", self(), {window, dim});
    }
    Series movmean() const { return _compat::invoke("movmean", self()); }
    Series movmean(std::int64_t window) const { return _compat::invoke("movmean", self(), {window}); }
    Series movmean(std::int64_t window, std::int64_t dim) const {
      return _compat::invoke("movmean", self(), {window, dim});
    }
    Series movmax() const { return _compat::invoke("movmax", self()); }
    Series movmax(std::int64_t window) const { return _compat::invoke("movmax", self(), {window}); }
    Series movmax(std::int64_t window, std::int64_t dim) const {
      return _compat::invoke("movmax", self(), {window, dim});
    }
    Series movmin() const { return _compat::invoke("movmin", self()); }
    Series movmin(std::int64_t window) const { return _compat::invoke("movmin", self(), {window}); }
    Series movmin(std::int64_t window, std::int64_t dim) const {
      return _compat::invoke("movmin", self(), {window, dim});
    }
    Series movstd() const { return _compat::invoke("movstd", self()); }
    Series movstd(std::int64_t window) const { return _compat::invoke("movstd", self(), {window}); }
    Series movstd(std::int64_t window, std::int64_t dim) const {
      return _compat::invoke("movstd", self(), {window, dim});
    }
    Series movvar() const { return _compat::invoke("movvar", self()); }
    Series movvar(std::int64_t window) const { return _compat::invoke("movvar", self(), {window}); }
    Series movvar(std::int64_t window, std::int64_t dim) const {
      return _compat::invoke("movvar", self(), {window, dim});
    }

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

  private:
    const Series& self() const {
      PLUSS_CHECK(self_ != nullptr, "Series.stat(): null self");
      return *self_;
    }

    const Series& peer1() const {
      PLUSS_CHECK(!peers_.empty(), "Series.stat(...): this operation requires one peer Series");
      return peers_.front();
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
    Series pdf() const { return (*this)().pdf(); }
    Series pdf(double sigma) const { return (*this)().pdf(sigma); }
    Series pdf(double mu, double sigma) const { return (*this)().pdf(mu, sigma); }
    Series logpdf() const { return (*this)().logpdf(); }
    Series logpdf(double sigma) const { return (*this)().logpdf(sigma); }
    Series logpdf(double mu, double sigma) const { return (*this)().logpdf(mu, sigma); }
    Series logcdf() const { return (*this)().logcdf(); }
    Series logcdf(double sigma) const { return (*this)().logcdf(sigma); }
    Series logcdf(double mu, double sigma) const { return (*this)().logcdf(mu, sigma); }
    Series cdf() const { return (*this)().cdf(); }
    Series cdf(double sigma) const { return (*this)().cdf(sigma); }
    Series cdf(double mu, double sigma) const { return (*this)().cdf(mu, sigma); }
    Series movsum() const { return (*this)().movsum(); }
    Series movsum(std::int64_t window) const { return (*this)().movsum(window); }
    Series movsum(std::int64_t window, std::int64_t dim) const { return (*this)().movsum(window, dim); }
    Series movmean() const { return (*this)().movmean(); }
    Series movmean(std::int64_t window) const { return (*this)().movmean(window); }
    Series movmean(std::int64_t window, std::int64_t dim) const { return (*this)().movmean(window, dim); }
    Series movmax() const { return (*this)().movmax(); }
    Series movmax(std::int64_t window) const { return (*this)().movmax(window); }
    Series movmax(std::int64_t window, std::int64_t dim) const { return (*this)().movmax(window, dim); }
    Series movmin() const { return (*this)().movmin(); }
    Series movmin(std::int64_t window) const { return (*this)().movmin(window); }
    Series movmin(std::int64_t window, std::int64_t dim) const { return (*this)().movmin(window, dim); }
    Series movstd() const { return (*this)().movstd(); }
    Series movstd(std::int64_t window) const { return (*this)().movstd(window); }
    Series movstd(std::int64_t window, std::int64_t dim) const { return (*this)().movstd(window, dim); }
    Series movvar() const { return (*this)().movvar(); }
    Series movvar(std::int64_t window) const { return (*this)().movvar(window); }
    Series movvar(std::int64_t window, std::int64_t dim) const { return (*this)().movvar(window, dim); }
    Series normr() const { return (*this)().normr(); }
    Series normc() const { return (*this)().normc(); }
    Series standardize() const { return (*this)().standardize(); }
    Series center() const { return (*this)().center(); }
    Series pdist2() const { return (*this)().pdist2(); }
    Series pdist2(const Series& other) const { return (*this)().pdist2(other); }
    Series cdist() const { return (*this)().cdist(); }
    Series cdist(const Series& other) const { return (*this)().cdist(other); }
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
    SeriesLinearAlgProxy(const Series& self, pluss::msharp::RangeView::Series others)
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
    SeriesLinearAlgProxy operator()(pluss::msharp::RangeView::Series others) const {
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
  private:
    Series& owner() const {
      PLUSS_CHECK(owner_ != nullptr, "Series.linearalg: unbound accessor");
      return *owner_;
    }

    Series* owner_{nullptr};
  };

  // Factory helpers (Torch-style free-functions live elsewhere; keep these for convenience).
  static Series undefined();
  static Series empty(pluss::msharp::RangeView::Int sizes, ScalarType st);
  static Series empty(pluss::msharp::RangeView::Int sizes, ScalarType st, Platform platform);
  static Series zeros(pluss::msharp::RangeView::Int sizes, ScalarType st);
  static Series ones(pluss::msharp::RangeView::Int sizes, ScalarType st);
  static Series full(pluss::msharp::RangeView::Int sizes, double value, ScalarType st);

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
  // For N-D shape-aware export, use host<T>().array().
  // Legacy user-facing API: prefer host<T>() / item<T>().
  // Retained during staged internal migration.
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

  // Host-export convenience entrypoint.
  // Typed variant:
  // - Scalar series materializes as scalar T
  // - 1-D series materializes as std::vector<T>
  // - N-D series materializes as HostArray<T> (data + sizes/strides)
  template <class T>
  HostValue<T> host() const {
    auto arr = detail_host_array_impl_<T>();
    if (numel() == 1) return HostValue<T>::from_scalar(std::move(arr));
    if (dim() <= 1) return HostValue<T>::from_vector(std::move(arr));
    return HostValue<T>::from_array(std::move(arr));
  }

  // Untyped variant:
  // - scalar -> HostScalarAny
  // - 1-D    -> HostVectorAny
  // - N-D    -> HostArrayAny
  // This does not require the caller to pass T.
  HostAny host() const {
    const auto st = scalar_type();
    switch (st) {
      case ScalarType::Bool: return detail_host_any_impl_<bool>(st);
      case ScalarType::UInt8: return detail_host_any_impl_<std::uint8_t>(st);
      case ScalarType::UInt16: return detail_host_any_impl_<std::uint16_t>(st);
      case ScalarType::UInt32: return detail_host_any_impl_<std::uint32_t>(st);
      case ScalarType::UInt64: return detail_host_any_impl_<std::uint64_t>(st);
      case ScalarType::Int8: return detail_host_any_impl_<std::int8_t>(st);
      case ScalarType::Int16: return detail_host_any_impl_<std::int16_t>(st);
      case ScalarType::Int32: return detail_host_any_impl_<std::int32_t>(st);
      case ScalarType::Int64: return detail_host_any_impl_<std::int64_t>(st);
      case ScalarType::Float16: return detail_host_any_impl_<Float16>(st);
      case ScalarType::BFloat16: return detail_host_any_impl_<BFloat16>(st);
      case ScalarType::Float32: return detail_host_any_impl_<float>(st);
      case ScalarType::Float64: return detail_host_any_impl_<double>(st);
      case ScalarType::Float8E4M3FN: return detail_host_any_float32_bridge_(st);
      case ScalarType::Float8E5M2: return detail_host_any_float32_bridge_(st);
      case ScalarType::Complex64: return detail_host_any_impl_<Complex64>(st);
      case ScalarType::Complex128: return detail_host_any_impl_<Complex128>(st);
      default:
        PLUSS_FAIL("Series::host(): unsupported dtype");
    }
  }

  [[nodiscard]] Attr attr() const {
    Attr out;
    out.shape.assign(sizes().begin(), sizes().end());
    out.strides.assign(strides().begin(), strides().end());
    out.dtype = scalar_type();
    out.platform = platform();
    out.numel = numel();
    out.contiguous = is_contiguous();
    out.values = host();
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
  ::pluss::StackArray<Series> max_pair(std::int64_t dim, bool keepdim=false) const;
  ::pluss::StackArray<Series> min_pair(std::int64_t dim, bool keepdim=false) const;
  ::pluss::StackArray<Series> argmax_pair(std::int64_t dim, bool keepdim=false) const;
  ::pluss::StackArray<Series> argmin_pair(std::int64_t dim, bool keepdim=false) const;
  ::pluss::StackArray<Series> sort_pair(std::int64_t dim=-1, bool descending=false, bool stable=false) const;
  ::pluss::StackArray<Series> topk_pair(std::int64_t k,
                                        std::int64_t dim=-1,
                                        bool largest=true,
                                        bool sorted=true) const;

  // Pluss autodiff entrypoint. This is a named wrapper over `_backward(...)`
  // so the public surface stays Pluss-native.
  void reverse() const { (void)pluss::codem::_compat::invoke("_backward", *this); }
  void reverse(bool keep_graph,
               bool accumulate_sensitivity = false,
               bool track_higher_order = false) const {
    (void)pluss::codem::_compat::invoke(
        "_backward", *this, {keep_graph, accumulate_sensitivity, track_higher_order});
  }
  void reverse(const Series& sensitivity_output) const {
    (void)pluss::codem::_compat::invoke("_backward", *this, {sensitivity_output});
  }
  void reverse(const Series& sensitivity_output,
               bool keep_graph,
               bool accumulate_sensitivity = false,
               bool track_higher_order = false) const {
    (void)pluss::codem::_compat::invoke(
        "_backward", *this, {sensitivity_output, keep_graph, accumulate_sensitivity, track_higher_order});
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

  Series count(bool count_true = true) const {
    PLUSS_CHECK(defined(), "Series::count: undefined Series");
    const auto target_platform = platform();
    const auto cpu = is_cpu() ? *this : to(Platform::CPU);

    std::int64_t total = 0;
    if (cpu.scalar_type() == ScalarType::Bool) {
      const auto values = cpu.to_vec<bool>();
      total = static_cast<std::int64_t>(std::count(values.begin(), values.end(), count_true));
    } else {
      PLUSS_CHECK(count_true, "Series::count(false) is only valid for Bool Series");
      total = cpu.numel();
    }

    auto out_cpu = Series::empty({1}, ScalarType::Int64, Platform::CPU);
    reinterpret_cast<std::int64_t*>(out_cpu.raw_data())[0] = total;
    return target_platform == Platform::CPU ? out_cpu : out_cpu.to(target_platform);
  }

private:

private:
  using ExtractSpec_ = ExtractSpec;

  static ExtractSpec_ make_extract_slice_(pluss::Slicing::Slice s) {
    ExtractSpec_ out;
    out.kind = ExtractSpec::Kind::Slice;
    out.slice = s;
    return out;
  }

  static bool extract_consumes_dim_(const ExtractSpec_& s) {
    return s.kind == ExtractSpec::Kind::Slice ||
           s.kind == ExtractSpec::Kind::ScalarIndex ||
           s.kind == ExtractSpec::Kind::AdvancedIndex;
  }

  static std::vector<ExtractSpec_> normalize_extract_specs_(const std::vector<ExtractSpec_>& specs,
                                                              std::int64_t rank) {
    std::int64_t consumed_without_ellipsis = 0;
    std::int64_t ellipsis_count = 0;
    for (const auto& s : specs) {
      if (s.kind == ExtractSpec::Kind::Ellipsis) {
        ++ellipsis_count;
      } else if (extract_consumes_dim_(s)) {
        ++consumed_without_ellipsis;
      }
    }
    PLUSS_CHECK(ellipsis_count <= 1, "Series::extract: at most one ellipsis is allowed");

    std::vector<ExtractSpec_> out;
    out.reserve(specs.size() + static_cast<std::size_t>(std::max<std::int64_t>(0, rank)));

    if (ellipsis_count == 0) {
      PLUSS_CHECK(consumed_without_ellipsis <= rank,
                  "Series::extract: too many indexing specs for Series rank");
      out = specs;
      const std::int64_t missing = rank - consumed_without_ellipsis;
      for (std::int64_t i = 0; i < missing; ++i) {
        out.push_back(make_extract_slice_(pluss::Slicing::SliceAll()));
      }
      return out;
    }

    const std::int64_t fill = rank - consumed_without_ellipsis;
    PLUSS_CHECK(fill >= 0, "Series::extract: ellipsis cannot compensate for too many indexing specs");
    for (const auto& s : specs) {
      if (s.kind != ExtractSpec::Kind::Ellipsis) {
        out.push_back(s);
        continue;
      }
      for (std::int64_t i = 0; i < fill; ++i) {
        out.push_back(make_extract_slice_(pluss::Slicing::SliceAll()));
      }
    }
    return out;
  }

  struct ExtractSliceBounds_ final {
    std::int64_t start{0};
    std::int64_t stop{0};
    std::int64_t step{1};
  };

  static std::int64_t normalize_scalar_index_(std::int64_t i, std::int64_t dim_size) {
    if (i < 0) i += dim_size;
    PLUSS_CHECK(i >= 0 && i < dim_size, "Series::extract: scalar index out of range");
    return i;
  }

  static ExtractSliceBounds_ normalize_slice_bounds_(const pluss::Slicing::Slice& s,
                                                      std::int64_t dim_size) {
    PLUSS_CHECK(dim_size >= 0, "Series::extract: invalid dimension size");
    PLUSS_CHECK(s.step != 0, "Series::extract: step cannot be 0");
    ExtractSliceBounds_ b;
    b.step = s.step;
    if (s.step > 0) {
      b.start =
          (s.start == pluss::Slicing::Slice::kUnbounded) ? 0 : (s.start < 0 ? s.start + dim_size : s.start);
      b.stop =
          (s.stop == pluss::Slicing::Slice::kUnbounded) ? dim_size : (s.stop < 0 ? s.stop + dim_size : s.stop);
      b.start = std::clamp<std::int64_t>(b.start, 0, dim_size);
      b.stop = std::clamp<std::int64_t>(b.stop, 0, dim_size);
    } else {
      b.start = (s.start == pluss::Slicing::Slice::kUnbounded) ? (dim_size - 1)
                                                                : (s.start < 0 ? s.start + dim_size : s.start);
      b.stop = (s.stop == pluss::Slicing::Slice::kUnbounded) ? -1 : (s.stop < 0 ? s.stop + dim_size : s.stop);
      b.start = std::clamp<std::int64_t>(b.start, -1, dim_size - 1);
      b.stop = std::clamp<std::int64_t>(b.stop, -1, dim_size - 1);
    }
    return b;
  }

  static bool is_identity_slice_(const ExtractSliceBounds_& b, std::int64_t dim_size) {
    return b.step == 1 && b.start == 0 && b.stop == dim_size;
  }

  static std::vector<std::int64_t> materialize_dim_indices_(const ExtractSpec_& spec,
                                                             std::int64_t dim_size) {
    PLUSS_CHECK(dim_size >= 0, "Series::extract: invalid dimension size");

    if (spec.kind == ExtractSpec::Kind::ScalarIndex) {
      return {normalize_scalar_index_(spec.scalar_index, dim_size)};
    }

    if (spec.kind == ExtractSpec::Kind::Slice) {
      const auto b = normalize_slice_bounds_(spec.slice, dim_size);

      std::vector<std::int64_t> out;
      if ((b.step > 0 && b.start >= b.stop) || (b.step < 0 && b.start <= b.stop)) {
        return out;
      }
      for (std::int64_t i = b.start; (b.step > 0) ? (i < b.stop) : (i > b.stop); i += b.step) {
        out.push_back(i);
      }
      return out;
    }

    PLUSS_CHECK(spec.kind == ExtractSpec::Kind::AdvancedIndex,
                "Series::extract: invalid internal spec kind");
    PLUSS_CHECK(spec.advanced_impl,
                "Series::extract: advanced index Series is undefined");
    Series idx_series;
    idx_series.impl_ = spec.advanced_impl;

    const Series idx_cpu = idx_series.to(Platform::CPU);
    if (idx_cpu.scalar_type() == ScalarType::Bool) {
      PLUSS_CHECK(idx_cpu.dim() == 1 && idx_cpu.size(0) == dim_size,
                  "Series::extract: boolean advanced index must be 1D and match the indexed dimension size");
      const auto mask = idx_cpu.to_vec<bool>();
      std::vector<std::int64_t> out;
      out.reserve(mask.size());
      for (std::int64_t i = 0; i < static_cast<std::int64_t>(mask.size()); ++i) {
        if (mask[static_cast<std::size_t>(i)]) out.push_back(i);
      }
      return out;
    }

    const auto st = idx_cpu.scalar_type();
    const bool is_integral = st == ScalarType::UInt8 || st == ScalarType::UInt16 ||
                             st == ScalarType::UInt32 || st == ScalarType::UInt64 ||
                             st == ScalarType::Int8 || st == ScalarType::Int16 ||
                             st == ScalarType::Int32 || st == ScalarType::Int64;
    PLUSS_CHECK(is_integral,
                "Series::extract: advanced index Series must be bool or integer dtype");
    const auto flat = idx_cpu.to(ScalarType::Int64).reshape({idx_cpu.numel()}).to_vec<std::int64_t>();
    std::vector<std::int64_t> out;
    out.reserve(flat.size());
    for (std::int64_t i : flat) {
      if (i < 0) i += dim_size;
      PLUSS_CHECK(i >= 0 && i < dim_size, "Series::extract: advanced index out of range");
      out.push_back(i);
    }
    return out;
  }

  static Series make_index_series_on_(std::int64_t dim_size, const ExtractSpec_& spec, Platform platform) {
    if (spec.kind == ExtractSpec::Kind::Slice) {
      const auto b = normalize_slice_bounds_(spec.slice, dim_size);
      const bool empty = (b.step > 0) ? (b.start >= b.stop) : (b.start <= b.stop);
      if (empty) {
        Series out = Series::empty({0}, ScalarType::Int64, Platform::CPU);
        return platform == Platform::CPU ? out : out.to(platform, ScalarType::Int64);
      }
      Series idx = Series::arange(
          static_cast<double>(b.start),
          static_cast<double>(b.stop),
          static_cast<double>(b.step),
          ScalarType::Int64);
      if (platform != Platform::CPU) {
        idx = idx.to(platform, ScalarType::Int64);
      }
      return idx;
    }

    const auto idx = materialize_dim_indices_(spec, dim_size);
    Series out = Series::empty(
        {static_cast<std::int64_t>(idx.size())},
        ScalarType::Int64,
        Platform::CPU);
    auto* p = reinterpret_cast<std::int64_t*>(out.raw_data());
    for (std::int64_t i = 0; i < static_cast<std::int64_t>(idx.size()); ++i) {
      p[i] = idx[static_cast<std::size_t>(i)];
    }
    return platform == Platform::CPU ? out : out.to(platform, ScalarType::Int64);
  }

  Series extract_from_specs_(const std::vector<ExtractSpec_>& specs) const {
    PLUSS_CHECK(defined(), "Series::extract: undefined Series");
    const auto normalized = normalize_extract_specs_(specs, dim());

    Series out = *this;
    std::int64_t dim_cursor = 0;
    for (const auto& spec : normalized) {
      if (spec.kind == ExtractSpec::Kind::NewAxis) {
        out = out.unsqueeze(dim_cursor);
        ++dim_cursor;
        continue;
      }
      PLUSS_CHECK(dim_cursor < out.dim(), "Series::extract: internal dimension cursor overflow");
      const std::int64_t n = out.size(dim_cursor);
      if (spec.kind == ExtractSpec::Kind::ScalarIndex) {
        out = out.select(dim_cursor, normalize_scalar_index_(spec.scalar_index, n));
        continue;
      }
      if (spec.kind == ExtractSpec::Kind::Slice) {
        const auto b = normalize_slice_bounds_(spec.slice, n);
        if (is_identity_slice_(b, n)) {
          ++dim_cursor;
          continue;
        }
        if (b.step > 0) {
          out = out.slice(dim_cursor, b.start, b.stop, b.step);
          ++dim_cursor;
          continue;
        }
      }
      const Series idx = make_index_series_on_(n, spec, out.platform());
      out = pluss::codem::_compat::invoke("index_select", out, {dim_cursor, idx});
      ++dim_cursor;
    }
    return out;
  }

  static bool can_traverse_as_view_(const std::vector<ExtractSpec_>& normalized) {
    for (const auto& spec : normalized) {
      if (spec.kind == ExtractSpec::Kind::AdvancedIndex) return false;
      if (spec.kind == ExtractSpec::Kind::Slice && spec.slice.step <= 0) return false;
    }
    return true;
  }

  Series traverse_slice_from_specs_(const std::vector<ExtractSpec_>& specs) const {
    PLUSS_CHECK(defined(), "Series::traverse_slice: undefined Series");
    const auto normalized = normalize_extract_specs_(specs, dim());

    if (!can_traverse_as_view_(normalized)) {
      PLUSS_FAIL(
          "Series::traverse_slice: selector is not view-representable; "
          "use extract() or extract_put_() for materializing paths");
    }

    Series out = *this;
    std::int64_t dim_cursor = 0;
    for (const auto& spec : normalized) {
      if (spec.kind == ExtractSpec::Kind::NewAxis) {
        out = out.unsqueeze(dim_cursor);
        ++dim_cursor;
        continue;
      }

      PLUSS_CHECK(dim_cursor < out.dim(), "Series::traverse_slice: internal dimension cursor overflow");
      const std::int64_t n = out.size(dim_cursor);

      if (spec.kind == ExtractSpec::Kind::ScalarIndex) {
        out = out.select(dim_cursor, normalize_scalar_index_(spec.scalar_index, n));
        continue;
      }

      if (spec.kind == ExtractSpec::Kind::Slice) {
        const auto b = normalize_slice_bounds_(spec.slice, n);
        if (is_identity_slice_(b, n)) {
          ++dim_cursor;
          continue;
        }
        out = out.slice(dim_cursor, b.start, b.stop, b.step);
        ++dim_cursor;
      }
    }
    return out;
  }

  Series& extract_put_from_specs_(const std::vector<ExtractSpec_>& specs, const Series& values) {
    PLUSS_CHECK(defined(), "Series::extract_put_: undefined Series");
    const auto normalized = normalize_extract_specs_(specs, dim());
    std::vector<Series> indices(static_cast<std::size_t>(dim()), Series::undefined());
    std::vector<Series> defined_raw;
    std::vector<std::int64_t> defined_dims;
    defined_raw.reserve(static_cast<std::size_t>(dim()));
    defined_dims.reserve(static_cast<std::size_t>(dim()));

    std::int64_t selected_numel = 1;
    std::int64_t src_dim = 0;
    for (const auto& spec : normalized) {
      if (spec.kind == ExtractSpec::Kind::NewAxis) continue;
      PLUSS_CHECK(src_dim < dim(), "Series::extract_put_: too many consuming specs");
      const auto dim_size = size(src_dim);

      if (spec.kind == ExtractSpec::Kind::Slice) {
        const auto b = normalize_slice_bounds_(spec.slice, dim_size);
        if (is_identity_slice_(b, dim_size)) {
          selected_numel *= dim_size;
          ++src_dim;
          continue;
        }
      }

      Series idx = make_index_series_on_(dim_size, spec, platform());
      if (idx.scalar_type() != ScalarType::Int64) {
        idx = idx.to(platform(), ScalarType::Int64);
      }
      selected_numel *= idx.numel();
      defined_raw.push_back(std::move(idx));
      defined_dims.push_back(src_dim);
      ++src_dim;
    }
    PLUSS_CHECK(
        src_dim == dim(),
        "Series::extract_put_: normalized specs did not consume all source dimensions");

    if (selected_numel == 0) return *this;

    if (!defined_raw.empty()) {
      const std::size_t k = defined_raw.size();
      for (std::size_t i = 0; i < k; ++i) {
        std::vector<std::int64_t> shape(k, 1);
        shape[i] = defined_raw[i].numel();
        indices[static_cast<std::size_t>(defined_dims[i])] =
            defined_raw[i].reshape(std::span<const std::int64_t>(shape.data(), shape.size()));
      }
    }

    Series src = values;
    if (src.scalar_type() != scalar_type() || src.platform() != platform()) {
      src = src.to(platform(), scalar_type());
    }
    PLUSS_CHECK(
        src.numel() == 1 || src.numel() == selected_numel,
        "Series::extract_put_: source must be scalar or match selected element count");
    if (src.numel() != 1) {
      src = src.reshape({selected_numel});
    }

    pluss::codem::_compat::invoke_(
        "index_put_",
        *this,
        {pluss::msharp::RangeView::Of<Series>(indices), src, false});
    return *this;
  }

  Series extract_put_copy_from_specs_(const std::vector<ExtractSpec_>& specs, const Series& values) const {
    PLUSS_CHECK(defined(), "Series::extract_put: undefined Series");
    const auto normalized = normalize_extract_specs_(specs, dim());
    std::vector<Series> indices(static_cast<std::size_t>(dim()), Series::undefined());
    std::vector<Series> defined_raw;
    std::vector<std::int64_t> defined_dims;
    defined_raw.reserve(static_cast<std::size_t>(dim()));
    defined_dims.reserve(static_cast<std::size_t>(dim()));

    std::int64_t selected_numel = 1;
    std::int64_t src_dim = 0;
    for (const auto& spec : normalized) {
      if (spec.kind == ExtractSpec::Kind::NewAxis) continue;
      PLUSS_CHECK(src_dim < dim(), "Series::extract_put: too many consuming specs");
      const auto dim_size = size(src_dim);

      if (spec.kind == ExtractSpec::Kind::Slice) {
        const auto b = normalize_slice_bounds_(spec.slice, dim_size);
        if (is_identity_slice_(b, dim_size)) {
          selected_numel *= dim_size;
          ++src_dim;
          continue;
        }
      }

      Series idx = make_index_series_on_(dim_size, spec, platform());
      if (idx.scalar_type() != ScalarType::Int64) {
        idx = idx.to(platform(), ScalarType::Int64);
      }
      selected_numel *= idx.numel();
      defined_raw.push_back(std::move(idx));
      defined_dims.push_back(src_dim);
      ++src_dim;
    }
    PLUSS_CHECK(
        src_dim == dim(),
        "Series::extract_put: normalized specs did not consume all source dimensions");

    Series out = clone();
    if (selected_numel == 0) return out;

    if (!defined_raw.empty()) {
      const std::size_t k = defined_raw.size();
      for (std::size_t i = 0; i < k; ++i) {
        std::vector<std::int64_t> shape(k, 1);
        shape[i] = defined_raw[i].numel();
        indices[static_cast<std::size_t>(defined_dims[i])] =
            defined_raw[i].reshape(std::span<const std::int64_t>(shape.data(), shape.size()));
      }
    }

    Series src = values;
    if (src.scalar_type() != scalar_type() || src.platform() != platform()) {
      src = src.to(platform(), scalar_type());
    }
    PLUSS_CHECK(
        src.numel() == 1 || src.numel() == selected_numel,
        "Series::extract_put: source must be scalar or match selected element count");
    if (src.numel() != 1) {
      src = src.reshape({selected_numel});
    }

    return pluss::codem::_compat::invoke(
        "index_put",
        out,
        {pluss::msharp::RangeView::Of<Series>(indices), src, false});
  }

public:
  // Traversal/view surface. Selector must be view-representable.
  Series traverse_slice(pluss::msharp::RangeView::Of<ExtractSpec> specs) const {
    return traverse_slice_from_specs_({specs.begin(), specs.end()});
  }

  Series& traverse_put_(pluss::msharp::RangeView::Of<ExtractSpec> specs, const Series& values) {
    Series view = traverse_slice(specs);
    pluss::codem::_compat::invoke_("copy_", view, {values});
    return *this;
  }

  Series traverse_put(pluss::msharp::RangeView::Of<ExtractSpec> specs, const Series& values) const {
    Series out = *this;
    Series view = out.traverse_slice(specs);
    pluss::codem::_compat::invoke_("copy_", view, {values});
    return out;
  }

  // Dynamic extract surface: accepts a dynamic list of specs.
  Series extract(pluss::msharp::RangeView::Of<ExtractSpec> specs) const {
    return extract_from_specs_({specs.begin(), specs.end()});
  }

  Series& extract_put_(pluss::msharp::RangeView::Of<ExtractSpec> specs, const Series& values) {
    return extract_put_from_specs_({specs.begin(), specs.end()}, values);
  }


  Series extract_put(pluss::msharp::RangeView::Of<ExtractSpec> specs, const Series& values) const {
    return extract_put_copy_from_specs_({specs.begin(), specs.end()}, values);
  }

  // PlussQit naming deviations (method-first aliases).
  Series pile(pluss::msharp::RangeView::Of<Series> others, std::int64_t dim=0) const;
  Series unite(pluss::msharp::RangeView::Of<Series> others, std::int64_t dim=0, bool strict_dtype=true) const;

  // Canonical indexing entrypoint. This accepts heterogeneous index specs
  // (advanced-index Series, scalar indices, SliceAll/SliceRange, NewAxis,
  // Ellipsis). Implementation lives in OpsInternal.cpp.
  Series index(pluss::msharp::RangeView::Of<ExtractSpec> specs) const;
  Series index_put(pluss::msharp::RangeView::Of<Series> indices, const Series& values, bool accumulate) const {
    return pluss::codem::_compat::invoke("index_put", *this, {indices, values, accumulate});
  }
  Series index_put(std::initializer_list<Series> indices, const Series& values, bool accumulate) const {
    return index_put(pluss::msharp::RangeView::Of<Series>(indices), values, accumulate);
  }
  Series index_put(std::int64_t index, const Series& values) const {
    Series out = *this;
    out.index_put_(index, values, false);
    return out;
  }
  Series& index_put_(pluss::msharp::RangeView::Of<Series> indices, const Series& values, bool accumulate) const {
    return pluss::codem::_compat::invoke_("index_put_", *this, {indices, values, accumulate});
  }
  Series& index_put_(std::initializer_list<Series> indices, const Series& values, bool accumulate) const {
    return index_put_(pluss::msharp::RangeView::Of<Series>(indices), values, accumulate);
  }

  // Torch-like scalar index convenience: x.index_put_(0, value)
  // This overload is intentionally 1D-only for explicitness.
  Series& index_put_(std::int64_t index, const Series& values, bool accumulate=false) const {
    PLUSS_CHECK(dim() == 1,
                "Series::index_put_(int64, value): only valid for 1D Series; for higher dimensions use index list or extract_put_");
    PLUSS_CHECK(values.numel() == 1,
                "Series::index_put_(int64, value): value must be scalar (numel==1)");
    const std::int64_t i = normalize_scalar_index_(index, size(0));
    Series idx = Series::empty({1}, ScalarType::Int64, Platform::CPU);
    reinterpret_cast<std::int64_t*>(idx.raw_data())[0] = i;
    if (platform() != Platform::CPU) {
      idx = idx.to(platform(), ScalarType::Int64);
    }
    const std::array<Series, 1> idx_arr{{idx}};
    return index_put_(pluss::msharp::RangeView::Of<Series>(idx_arr), values, accumulate);
  }

  // Slice-spec index convenience for common 1D/2D scalar assignment forms:
  // - 1D: x.index_put_({i}, value)
  // - 2D: x.index_put_({SliceAll(), j}, col_values)
  // For richer N-D slicing/writes use extract_put_ directly.
  Series& index_put_(std::initializer_list<ExtractSpec> specs, const Series& values, bool accumulate=false) const {
    PLUSS_CHECK(!accumulate,
                "Series::index_put_(specs, value, accumulate): accumulate=true is unsupported for slice-spec indexing; use Series index list overload instead");
    const std::vector<ExtractSpec> parsed(specs.begin(), specs.end());
    PLUSS_CHECK(!parsed.empty(),
                "Series::index_put_(specs, value): specs cannot be empty");
    PLUSS_CHECK(parsed.size() <= 2,
                "Series::index_put_(specs, value): only 1D and 2D convenience specs are supported; use extract_put_ for higher-dimensional writes");

    if (parsed.size() == 1) {
      PLUSS_CHECK(dim() == 1,
                  "Series::index_put_({i}, value): target must be 1D");
      PLUSS_CHECK(parsed[0].kind == ExtractSpec::Kind::ScalarIndex,
                  "Series::index_put_({i}, value): spec must be a scalar index");
      PLUSS_CHECK(values.numel() == 1,
                  "Series::index_put_({i}, value): value must be scalar (numel==1)");
      return const_cast<Series*>(this)->extract_put_(parsed, values);
    }

    PLUSS_CHECK(dim() == 2,
                "Series::index_put_({SliceAll(), j}, value): target must be 2D");
    PLUSS_CHECK(parsed[0].kind == ExtractSpec::Kind::Slice && parsed[0].slice.is_all(),
                "Series::index_put_({SliceAll(), j}, value): first spec must be SliceAll()");
    PLUSS_CHECK(parsed[1].kind == ExtractSpec::Kind::ScalarIndex,
                "Series::index_put_({SliceAll(), j}, value): second spec must be a scalar column index");
    PLUSS_CHECK(values.dim() == 1 && values.numel() == size(0),
                "Series::index_put_({SliceAll(), j}, value): value must be 1D with length equal to size(0)");
    return const_cast<Series*>(this)->extract_put_(parsed, values);
  }

  Series slice(std::int64_t dim, std::int64_t start, std::int64_t end) const {
    return slice(dim, start, end, 1);
  }

  Series slice(std::int64_t dim, std::int64_t start) const {
    return slice(dim, start, std::numeric_limits<std::int64_t>::max(), 1);
  }

  Series slice(std::int64_t dim) const {
    return slice(dim, 0, std::numeric_limits<std::int64_t>::max(), 1);
  }

  // Statistical accessor:
  // - safe call style: `s.stat().mean()`
  // - convenience dot style: `s.stat.mean()`
  SeriesStatAccessor stat;
  // Linear algebra accessor:
  // - safe call style: `s.linearalg().det()`
  // - convenience dot style: `s.linearalg.det()`
  SeriesLinearAlgAccessor linearalg;
  // Traverse/query accessor:
  // - dot style: `s.traverse.find_all(mask)`
  // - with explicit `on` selector: `s.traverse(specs).find_ge(...)`
  SeriesTraverseAccessor traverse;

  Series() : SeriesBase(), stat(*this), linearalg(*this), traverse(*this) {}
  Series(const Series& other) : SeriesBase(other), stat(*this), linearalg(*this), traverse(*this) {}
  Series(Series&& other) noexcept : SeriesBase(std::move(other)), stat(*this), linearalg(*this), traverse(*this) {}
  Series& operator=(const Series& other) {
    if (this != &other) {
      SeriesBase::operator=(other);
    }
    stat.rebind(*this);
    linearalg.rebind(*this);
    traverse.rebind(*this);
    return *this;
  }
  Series& operator=(Series&& other) noexcept {
    if (this != &other) {
      SeriesBase::operator=(std::move(other));
    }
    stat.rebind(*this);
    linearalg.rebind(*this);
    traverse.rebind(*this);
    return *this;
  }

private:
  template <class>
  struct detail_false_ : std::false_type {};

  template <class T>
  HostArray<T> detail_host_array_impl_() const {
    HostArray<T> out;
    out.sizes.assign(sizes().begin(), sizes().end());
    out.strides.assign(strides().begin(), strides().end());
    out.contiguous = is_contiguous();
    out.data = detail_materialize_vec_<T>();
    return out;
  }

  template <class T>
  HostAny detail_host_any_impl_(ScalarType st) const {
    auto arr = detail_host_array_impl_<T>();
    if (numel() == 1) {
      PLUSS_CHECK(!arr.data.empty(), "Series::host(): scalar payload is empty");
      return HostAny::from_scalar(HostScalarAny{arr.data.front()}, st);
    }
    if (dim() <= 1) {
      return HostAny::from_vector(HostVectorAny{arr.data}, st);
    }
    HostArrayAny any_arr{
        .data = HostVectorAny{std::move(arr.data)},
        .sizes = std::move(arr.sizes),
        .strides = std::move(arr.strides),
        .contiguous = arr.contiguous,
    };
    return HostAny::from_array(std::move(any_arr), st);
  }

  HostAny detail_host_any_float32_bridge_(ScalarType original_dtype) const {
    const Series cpu_f32 = to(Platform::CPU, ScalarType::Float32);
    auto arr = cpu_f32.detail_host_array_impl_<float>();
    if (cpu_f32.numel() == 1) {
      PLUSS_CHECK(!arr.data.empty(), "Series::host(): scalar payload is empty");
      return HostAny::from_scalar(HostScalarAny{arr.data.front()}, original_dtype);
    }
    if (cpu_f32.dim() <= 1) {
      return HostAny::from_vector(HostVectorAny{arr.data}, original_dtype);
    }
    HostArrayAny any_arr{
        .data = HostVectorAny{std::move(arr.data)},
        .sizes = std::move(arr.sizes),
        .strides = std::move(arr.strides),
        .contiguous = arr.contiguous,
    };
    return HostAny::from_array(std::move(any_arr), original_dtype);
  }

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
      : SeriesBase(std::move(impl)), stat(*this), linearalg(*this), traverse(*this) {}

  // Internal construction hook used by ops_internal implementations.
  friend struct ops_internal::SeriesFactoryAccess;
};


inline ExtractSpec::ExtractSpec(const Series& idx) : kind(Kind::AdvancedIndex) {
  PLUSS_CHECK(idx.defined(), "Series::extract: advanced index Series is undefined");
  advanced_impl = idx.impl();
}

} // namespace pluss::codem

#include "CodeM/Traverse/TraverseTypes.h"

// Now that `pluss::codem::Series` is a complete type, include the compat
// header again to compile the template helper definitions that rely on the
// complete `Series` definition. The compat header intentionally exposes the
// templates only when `PLUSS_CODEM_SERIES_COMPLETE` is defined on a second
// include.
#define PLUSS_CODEM_SERIES_COMPLETE
#include "CodeM/compat/Invoke.h"
#undef PLUSS_CODEM_SERIES_COMPLETE
