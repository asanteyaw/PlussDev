#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <functional>
#include <initializer_list>
#include <iosfwd>
#include <limits>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "CodeM/Indexing/ExtractSpec.h"
#include "CodeM/Traverse/Traverse.h"
#include "CodeM/core/Errors.h"

#include "CodeM/core/Complex.h"
#include "CodeM/core/DType.h"
#include "CodeM/core/Half.h"
#include "CodeM/core/Scalar.h"
#include "CodeM/core/ScalarType.h"
#include "CodeM/core/SeriesBase.h"
#include "CodeM/core/Storage.h"
#include "CodeM/core/exotic_containers.h"
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
template <class T> class StackArray;
}

namespace pluss::codem {
class Series;
}

namespace pluss::series {
CODEM_API codem::Series undefined();
}

namespace pluss::codem {

namespace host_detail {

template <class T> struct NativeRank {
  using leaf_type = std::remove_cv_t<T>;
  static constexpr std::size_t value = 0;
};

template <class T, class Allocator>
struct NativeRank<std::vector<T, Allocator>> {
  using leaf_type = typename NativeRank<T>::leaf_type;
  static constexpr std::size_t value = NativeRank<T>::value + 1;
};

} // namespace host_detail

namespace ops_internal {
struct SeriesFactoryAccess;
} // namespace ops_internal

// Stochastic-calculus value types used by `Series::calc.diff.ito(...)` and
// friends; defined after `Series` (they hold Series members).
struct SeriesItoState;
struct SeriesJumpMeasure;
struct SeriesItoDifferential;
struct SeriesItoOptions;

struct TernaryCompareBuilder;

// Forward declaration for tensor memory/layout format used by generated APIs.
// Full definition should come from the appropriate header if/when needed.
enum class MemoryFormat : std::uint8_t;

// User-facing Series (Torch-like Tensor).
class CODEM_API Series : public SeriesBase {
public:
  using SeriesBase::size;
  std::int64_t size(int dim) const {
    return SeriesBase::size(static_cast<std::int64_t>(dim));
  }
  std::int64_t size(std::int64_t dim) const { return SeriesBase::size(dim); }

  template <class T> struct HostArray {
    std::vector<T> data;
    std::vector<std::int64_t> sizes;
    std::vector<std::int64_t> strides;
    bool contiguous{false};

    std::span<const std::int64_t> shape() const noexcept { return sizes; }
    std::span<const std::int64_t> stride_view() const noexcept {
      return strides;
    }
    std::int64_t dim() const noexcept {
      return static_cast<std::int64_t>(sizes.size());
    }
    std::int64_t numel() const noexcept {
      return static_cast<std::int64_t>(data.size());
    }
  };

  using HostScalarAny =
      std::variant<bool, std::uint8_t, std::uint16_t, std::uint32_t,
                   std::uint64_t, std::int8_t, std::int16_t, std::int32_t,
                   std::int64_t, Float16, BFloat16, float, double, Complex64,
                   Complex128>;

  using HostVectorAny = std::variant<
      std::vector<bool>, std::vector<std::uint8_t>, std::vector<std::uint16_t>,
      std::vector<std::uint32_t>, std::vector<std::uint64_t>,
      std::vector<std::int8_t>, std::vector<std::int16_t>,
      std::vector<std::int32_t>, std::vector<std::int64_t>,
      std::vector<Float16>, std::vector<BFloat16>, std::vector<float>,
      std::vector<double>, std::vector<Complex64>, std::vector<Complex128>>;

  struct HostArrayAny final {
    HostVectorAny data;
    std::vector<std::int64_t> sizes;
    std::vector<std::int64_t> strides;
    bool contiguous{false};

    std::span<const std::int64_t> shape() const noexcept { return sizes; }
    std::span<const std::int64_t> stride_view() const noexcept {
      return strides;
    }
    std::int64_t dim() const noexcept {
      return static_cast<std::int64_t>(sizes.size());
    }
    std::int64_t numel() const noexcept {
      return std::visit(
          [](const auto &v) -> std::int64_t {
            return static_cast<std::int64_t>(v.size());
          },
          data);
    }

    template <class T> std::span<const T> flat_span_as() const {
      const auto &v = std::get<std::vector<T>>(data);
      if constexpr (std::is_same_v<T, bool>) {
        PLUSS_FAIL("Series::host(): bool payload does not expose span view; "
                   "use vector_as<bool>()");
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

    const HostScalarAny &scalar() const {
      PLUSS_CHECK(is_scalar(), "Series::host(): value is not scalar");
      return std::get<HostScalarAny>(payload_);
    }

    const HostVectorAny &vector() const {
      PLUSS_CHECK(is_vector(), "Series::host(): value is not a 1-D vector");
      return std::get<HostVectorAny>(payload_);
    }

    const HostArrayAny &array() const {
      PLUSS_CHECK(is_array(), "Series::host(): value is not N-D array");
      return std::get<HostArrayAny>(payload_);
    }

    template <class T> const T &scalar_as() const {
      return std::get<T>(scalar());
    }

    template <class T> const std::vector<T> &vector_as() const {
      return std::get<std::vector<T>>(vector());
    }

    template <class T> std::span<const T> vector_span_as() const {
      const auto &v = vector_as<T>();
      if constexpr (std::is_same_v<T, bool>) {
        PLUSS_FAIL("Series::host(): bool payload does not expose span view; "
                   "use vector_as<bool>()");
      } else {
        return std::span<const T>(v.data(), v.size());
      }
    }

    template <class T> std::span<const T> array_flat_span_as() const {
      return array().template flat_span_as<T>();
    }

    template <class T> T to_scalar() const {
      PLUSS_CHECK(is_scalar(),
                  "Series::host(): scalar conversion requested on non-scalar");
      return std::visit(
          [](const auto &v) -> T {
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
      PLUSS_CHECK(
          is_scalar(),
          "Series::host(): scalar numeric conversion requested on non-scalar");
      return std::visit(
          [](const auto &v) -> double {
            using U = std::remove_cv_t<std::remove_reference_t<decltype(v)>>;
            if constexpr (std::is_same_v<U, bool>) {
              return v ? 1.0 : 0.0;
            } else if constexpr (std::is_same_v<U, Float16>) {
              return static_cast<double>(float16_to_float(v));
            } else if constexpr (std::is_same_v<U, BFloat16>) {
              return static_cast<double>(bfloat16_to_float(v));
            } else if constexpr (std::is_same_v<U, Complex64> ||
                                 std::is_same_v<U, Complex128>) {
              PLUSS_CHECK(std::abs(v.imag()) == 0.0,
                          "Series::host(): cannot cast complex scalar with "
                          "non-zero imaginary part to double");
              return static_cast<double>(v.real());
            } else {
              return static_cast<double>(v);
            }
          },
          scalar());
    }

    operator double() const { return to_double(); }

    friend std::ostream &operator<<(std::ostream &os, Kind k) {
      switch (k) {
      case Kind::Scalar:
        os << "Scalar";
        break;
      case Kind::Vector:
        os << "Vector";
        break;
      case Kind::Array:
        os << "Array";
        break;
      }
      return os;
    }

    friend std::ostream &operator<<(std::ostream &os, const HostAny &v) {
      auto append_scalar = [&](const auto &x) {
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

      auto append_vector = [&](const auto &vec) {
        os << "[";
        for (std::size_t i = 0; i < vec.size(); ++i) {
          if (i)
            os << ", ";
          append_scalar(vec[i]);
        }
        os << "]";
      };

      if (v.is_scalar()) {
        std::visit([&](const auto &x) { append_scalar(x); }, v.scalar());
        return os;
      }
      if (v.is_vector()) {
        std::visit([&](const auto &vec) { append_vector(vec); }, v.vector());
        return os;
      }
      const auto &arr = v.array();
      os << "{shape=[";
      for (std::size_t i = 0; i < arr.sizes.size(); ++i) {
        if (i)
          os << ",";
        os << arr.sizes[i];
      }
      os << "], flat=";
      std::visit([&](const auto &vec) { append_vector(vec); }, arr.data);
      os << "}";
      return os;
    }

  private:
    Kind kind_{Kind::Scalar};
    ScalarType dtype_{ScalarType::Float64};
    std::variant<HostScalarAny, HostVectorAny, HostArrayAny> payload_{
        HostScalarAny{0.0}};
  };

  // Dynamic host export wrapper:
  // - scalar Series -> scalar T
  // - 1-D Series    -> std::vector<T>
  // - N-D Series    -> HostArray<T>
  // `array()` always preserves exact shape/stride/contiguous metadata from
  // the source Series (including scalar/1-D cases).
  template <class T> class HostValue {
  public:
    enum class Kind : std::uint8_t { Scalar = 0, Vector = 1, Array = 2 };
    using value_type = T;
    using size_type = typename std::vector<T>::size_type;
    using reference = typename std::vector<T>::reference;
    using const_reference = typename std::vector<T>::const_reference;
    using iterator = typename std::vector<T>::iterator;
    using const_iterator = typename std::vector<T>::const_iterator;

    HostValue() = default;
    explicit HostValue(HostArray<T> arr, Kind kind = Kind::Array)
        : arr_(std::move(arr)), kind_(kind) {}

    static HostValue from_scalar(HostArray<T> arr) {
      return HostValue(std::move(arr), Kind::Scalar);
    }
    static HostValue from_vector(HostArray<T> arr) {
      return HostValue(std::move(arr), Kind::Vector);
    }
    static HostValue from_array(HostArray<T> arr) {
      return HostValue(std::move(arr), Kind::Array);
    }

    Kind kind() const noexcept { return kind_; }
    bool is_scalar() const noexcept { return kind_ == Kind::Scalar; }
    bool is_vector() const noexcept { return kind_ == Kind::Vector; }
    bool is_array() const noexcept { return kind_ == Kind::Array; }

    T scalar() const {
      PLUSS_CHECK(is_scalar(), "Series::HostValue: not scalar");
      PLUSS_CHECK(
          arr_.data.size() == 1,
          "Series::HostValue: scalar payload must contain exactly one element");
      return arr_.data.front();
    }

    const HostArray<T> &array() const { return arr_; }

    // A HostValue retained through `auto` remains a directly readable flat
    // row-major host range. Assign it to a nested std::vector type when the
    // native value must preserve Series rank.
    bool empty() const noexcept { return arr_.data.empty(); }
    size_type size() const noexcept { return arr_.data.size(); }
    reference operator[](size_type i) { return arr_.data[i]; }
    const_reference operator[](size_type i) const { return arr_.data[i]; }
    reference at(size_type i) { return arr_.data.at(i); }
    const_reference at(size_type i) const { return arr_.data.at(i); }
    reference front() { return arr_.data.front(); }
    const_reference front() const { return arr_.data.front(); }
    reference back() { return arr_.data.back(); }
    const_reference back() const { return arr_.data.back(); }
    iterator begin() noexcept { return arr_.data.begin(); }
    const_iterator begin() const noexcept { return arr_.data.begin(); }
    const_iterator cbegin() const noexcept { return arr_.data.cbegin(); }
    iterator end() noexcept { return arr_.data.end(); }
    const_iterator end() const noexcept { return arr_.data.end(); }
    const_iterator cend() const noexcept { return arr_.data.cend(); }
    T *data() noexcept
      requires(!std::is_same_v<T, bool>)
    {
      return arr_.data.data();
    }
    const T *data() const noexcept
      requires(!std::is_same_v<T, bool>)
    {
      return arr_.data.data();
    }

    operator T() const { return scalar(); }
    template <class Native>
      requires(host_detail::NativeRank<Native>::value > 0 &&
               std::is_same_v<
                   typename host_detail::NativeRank<Native>::leaf_type, T>)
    operator Native() const {
      constexpr auto native_rank = host_detail::NativeRank<Native>::value;
      if constexpr (native_rank == 1) {
        PLUSS_CHECK(is_vector(),
                    "Series::host<T>(): a flat vector destination requires "
                    "a rank-one Series; use pull<T>()");
        return Native(arr_.data.begin(), arr_.data.end());
      }
      PLUSS_CHECK(
          arr_.sizes.size() == native_rank,
          "Series::host<T>(): destination nesting depth must match Series rank");
      std::size_t offset = 0;
      Native out = build_native_<Native>(0, offset);
      PLUSS_CHECK(offset == arr_.data.size(),
                  "Series::host<T>(): shape does not match materialized data");
      return out;
    }
    operator HostArray<T>() const { return arr_; }

    friend bool operator==(const HostValue &lhs, const HostValue &rhs) {
      return lhs.kind_ == rhs.kind_ && lhs.arr_.data == rhs.arr_.data &&
             lhs.arr_.sizes == rhs.arr_.sizes &&
             lhs.arr_.strides == rhs.arr_.strides &&
             lhs.arr_.contiguous == rhs.arr_.contiguous;
    }
    friend bool operator==(const HostValue &lhs, const std::vector<T> &rhs) {
      return lhs.arr_.data == rhs;
    }
    friend bool operator==(const std::vector<T> &lhs, const HostValue &rhs) {
      return lhs == rhs.arr_.data;
    }

    friend std::ostream &operator<<(std::ostream &os, const HostValue &v) {
      auto append_scalar = [&](const auto &x) {
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

      auto append_vector = [&](const std::vector<T> &vec) {
        os << "[";
        for (std::size_t i = 0; i < vec.size(); ++i) {
          if (i)
            os << ", ";
          append_scalar(vec[i]);
        }
        os << "]";
      };

      if (v.is_scalar()) {
        append_scalar(v.scalar());
        return os;
      }
      if (v.is_vector()) {
        append_vector(v.arr_.data);
        return os;
      }
      const auto &arr = v.array();
      os << "{shape=[";
      for (std::size_t i = 0; i < arr.sizes.size(); ++i) {
        if (i)
          os << ",";
        os << arr.sizes[i];
      }
      os << "], flat=";
      append_vector(arr.data);
      os << "}";
      return os;
    }

  private:
    template <class Native>
    Native build_native_(std::size_t axis, std::size_t &offset) const {
      if constexpr (host_detail::NativeRank<Native>::value == 0) {
        PLUSS_CHECK(offset < arr_.data.size(),
                    "Series::host<T>(): shape exceeds materialized data");
        return static_cast<Native>(arr_.data[offset++]);
      } else {
        PLUSS_CHECK(axis < arr_.sizes.size(),
                    "Series::host<T>(): invalid nested host rank");
        const auto extent = arr_.sizes[axis];
        PLUSS_CHECK(extent >= 0,
                    "Series::host<T>(): negative Series extent");
        Native out;
        out.reserve(static_cast<std::size_t>(extent));
        for (std::int64_t i = 0; i < extent; ++i) {
          out.emplace_back(
              build_native_<typename Native::value_type>(axis + 1, offset));
        }
        return out;
      }
    }

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

    [[nodiscard]] std::span<const std::int64_t> shape_view() const noexcept {
      return shape;
    }
    [[nodiscard]] std::span<const std::int64_t> stride_view() const noexcept {
      return strides;
    }
    [[nodiscard]] std::int64_t rank() const noexcept {
      return static_cast<std::int64_t>(shape.size());
    }
    [[nodiscard]] ScalarType scalar_type() const noexcept { return dtype; }
    [[nodiscard]] Platform device() const noexcept { return platform; }
    [[nodiscard]] bool is_contiguous() const noexcept { return contiguous; }
    [[nodiscard]] bool is_scalar() const noexcept { return numel == 1; }
    [[nodiscard]] bool is_vector() const noexcept {
      return rank() <= 1 && numel > 1;
    }
    [[nodiscard]] bool is_array() const noexcept { return rank() > 1; }
    [[nodiscard]] HostAny::Kind value_kind() const noexcept {
      return values.kind();
    }

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

    friend std::ostream &operator<<(std::ostream &os, const Attr &a) {
      os << a.to_string();
      return os;
    }

  private:
    static std::string dims_to_string_(const std::vector<std::int64_t> &xs) {
      std::ostringstream oss;
      oss << "[";
      for (std::size_t i = 0; i < xs.size(); ++i) {
        if (i)
          oss << ",";
        oss << xs[i];
      }
      oss << "]";
      return oss.str();
    }

    template <class U>
    static void append_scalar_(std::ostringstream &oss, const U &v) {
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
    static void append_vector_preview_(std::ostringstream &oss, const VecT &v,
                                       std::int64_t edge_items) {
      const std::size_t n = v.size();
      const std::size_t k =
          static_cast<std::size_t>(std::max<std::int64_t>(1, edge_items));
      oss << "[";
      if (n <= 2 * k) {
        for (std::size_t i = 0; i < n; ++i) {
          if (i)
            oss << ", ";
          append_scalar_(oss, v[i]);
        }
      } else {
        for (std::size_t i = 0; i < k; ++i) {
          if (i)
            oss << ", ";
          append_scalar_(oss, v[i]);
        }
        oss << ", ..., ";
        for (std::size_t i = n - k; i < n; ++i) {
          if (i > n - k)
            oss << ", ";
          append_scalar_(oss, v[i]);
        }
      }
      oss << "]";
    }

    static std::string host_any_to_string_(const HostAny &any,
                                           std::int64_t edge_items) {
      std::ostringstream oss;
      if (any.is_scalar()) {
        std::visit([&](const auto &x) { append_scalar_(oss, x); },
                   any.scalar());
        return oss.str();
      }
      if (any.is_vector()) {
        std::visit(
            [&](const auto &v) { append_vector_preview_(oss, v, edge_items); },
            any.vector());
        return oss.str();
      }
      const auto &arr = any.array();
      oss << "{shape=" << dims_to_string_(arr.sizes) << ", flat=";
      std::visit(
          [&](const auto &v) { append_vector_preview_(oss, v, edge_items); },
          arr.data);
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
    explicit SeriesStatProxy(const Series &self) : self_(&self) {}
    SeriesStatProxy(const Series &self, const Series &other)
        : self_(&self), peers_{other} {}
    SeriesStatProxy(const Series &self, std::initializer_list<Series> others)
        : self_(&self), peers_(others.begin(), others.end()) {}
    SeriesStatProxy(const Series &self, std::span<const Series> others)
        : self_(&self), peers_(others.begin(), others.end()) {}

    // Central tendency / spread
    Series mean() const { return pluss::subsp::stats::mean(self()); }
    Series mean(std::int64_t dim, bool keepdim = false) const {
      return pluss::subsp::stats::mean(self(), dim, keepdim);
    }
    Series var(bool unbiased = true) const {
      return pluss::subsp::stats::var(self(), unbiased);
    }
    Series var(std::int64_t dim, bool unbiased = true,
               bool keepdim = false) const {
      return pluss::subsp::stats::var(self(), dim, unbiased, keepdim);
    }
    Series std(bool unbiased = true) const {
      return pluss::subsp::stats::std(self(), unbiased);
    }
    Series std(std::int64_t dim, bool unbiased = true,
               bool keepdim = false) const {
      return pluss::subsp::stats::std(self(), dim, unbiased, keepdim);
    }
    Series nanvar() const { return pluss::subsp::stats::nanvar(self()); }
    Series nanvar(std::int64_t dim, bool keepdim = false) const {
      return pluss::subsp::stats::nanvar(self(), dim, keepdim);
    }
    Series nanstd() const { return pluss::subsp::stats::nanstd(self()); }
    Series nanstd(std::int64_t dim, bool keepdim = false) const {
      return pluss::subsp::stats::nanstd(self(), dim, keepdim);
    }
    Series skewness() const { return pluss::subsp::stats::skewness(self()); }
    Series skewness(std::int64_t dim) const {
      return pluss::subsp::stats::skewness(self(), dim);
    }
    Series kurtosis() const { return pluss::subsp::stats::kurtosis(self()); }
    Series kurtosis(std::int64_t dim) const {
      return pluss::subsp::stats::kurtosis(self(), dim);
    }

    // Distribution helpers (kept on stat surface; not on core Series surface).
    Series pdf() const { return _compat::invoke("pdf", self()); }
    Series pdf(double sigma) const {
      return _compat::invoke("pdf", self(), {sigma});
    }
    Series pdf(double mu, double sigma) const {
      return _compat::invoke("pdf", self(), {mu, sigma});
    }

    Series logpdf() const { return _compat::invoke("logpdf", self()); }
    Series logpdf(double sigma) const {
      return _compat::invoke("logpdf", self(), {sigma});
    }
    Series logpdf(double mu, double sigma) const {
      return _compat::invoke("logpdf", self(), {mu, sigma});
    }

    Series logcdf() const { return _compat::invoke("logcdf", self()); }
    Series logcdf(double sigma) const {
      return _compat::invoke("logcdf", self(), {sigma});
    }
    Series logcdf(double mu, double sigma) const {
      return _compat::invoke("logcdf", self(), {mu, sigma});
    }

    Series cdf() const { return _compat::invoke("cdf", self()); }
    Series cdf(double sigma) const {
      return _compat::invoke("cdf", self(), {sigma});
    }
    Series cdf(double mu, double sigma) const {
      return _compat::invoke("cdf", self(), {mu, sigma});
    }

    // Rolling stats
    Series movsum() const { return _compat::invoke("movsum", self()); }
    Series movsum(std::int64_t window) const {
      return _compat::invoke("movsum", self(), {window});
    }
    Series movsum(std::int64_t window, std::int64_t dim) const {
      return _compat::invoke("movsum", self(), {window, dim});
    }
    Series movmean() const { return _compat::invoke("movmean", self()); }
    Series movmean(std::int64_t window) const {
      return _compat::invoke("movmean", self(), {window});
    }
    Series movmean(std::int64_t window, std::int64_t dim) const {
      return _compat::invoke("movmean", self(), {window, dim});
    }
    Series movmax() const { return _compat::invoke("movmax", self()); }
    Series movmax(std::int64_t window) const {
      return _compat::invoke("movmax", self(), {window});
    }
    Series movmax(std::int64_t window, std::int64_t dim) const {
      return _compat::invoke("movmax", self(), {window, dim});
    }
    Series movmin() const { return _compat::invoke("movmin", self()); }
    Series movmin(std::int64_t window) const {
      return _compat::invoke("movmin", self(), {window});
    }
    Series movmin(std::int64_t window, std::int64_t dim) const {
      return _compat::invoke("movmin", self(), {window, dim});
    }
    Series movstd() const { return _compat::invoke("movstd", self()); }
    Series movstd(std::int64_t window) const {
      return _compat::invoke("movstd", self(), {window});
    }
    Series movstd(std::int64_t window, std::int64_t dim) const {
      return _compat::invoke("movstd", self(), {window, dim});
    }
    Series movvar() const { return _compat::invoke("movvar", self()); }
    Series movvar(std::int64_t window) const {
      return _compat::invoke("movvar", self(), {window});
    }
    Series movvar(std::int64_t window, std::int64_t dim) const {
      return _compat::invoke("movvar", self(), {window, dim});
    }

    // Normalization-style stats
    Series normr() const { return pluss::subsp::stats::normr(self()); }
    Series normc() const { return pluss::subsp::stats::normc(self()); }
    Series standardize() const {
      return pluss::subsp::stats::standardize(self());
    }
    Series standardize(std::int64_t dim) const {
      return pluss::subsp::stats::standardize(self(), dim);
    }
    Series center() const { return pluss::subsp::stats::center(self()); }
    Series center(std::int64_t dim) const {
      return pluss::subsp::stats::center(self(), dim);
    }

    // Pairwise stats (require one peer Series either bound via stat(peer) or
    // passed directly).
    Series pdist2() const {
      return pluss::subsp::stats::pdist2(self(), peer1());
    }
    Series pdist2(const Series &other) const {
      return pluss::subsp::stats::pdist2(self(), other);
    }
    Series cdist() const { return pluss::subsp::stats::cdist(self(), peer1()); }
    Series cdist(const Series &other) const {
      return pluss::subsp::stats::cdist(self(), other);
    }

  private:
    const Series &self() const {
      PLUSS_CHECK(self_ != nullptr, "Series.stat(): null self");
      return *self_;
    }

    const Series &peer1() const {
      PLUSS_CHECK(!peers_.empty(),
                  "Series.stat(...): this operation requires one peer Series");
      return peers_.front();
    }

    const Series *self_{nullptr};
    std::vector<Series> peers_{};
  };

  class SeriesStatAccessor final {
  public:
    SeriesStatAccessor() = default;
    explicit SeriesStatAccessor(Series &owner) : owner_(&owner) {}

    void rebind(Series &owner) noexcept { owner_ = &owner; }

    SeriesStatProxy operator()() const { return SeriesStatProxy(owner()); }
    SeriesStatProxy operator()(const Series &other) const {
      return SeriesStatProxy(owner(), other);
    }
    SeriesStatProxy operator()(std::initializer_list<Series> others) const {
      return SeriesStatProxy(owner(), others);
    }
    SeriesStatProxy operator()(std::span<const Series> others) const {
      return SeriesStatProxy(owner(), others);
    }

    // Dot-style convenience for no-arg stat operations.
    Series mean() const { return (*this)().mean(); }
    Series mean(std::int64_t dim, bool keepdim = false) const {
      return (*this)().mean(dim, keepdim);
    }
    Series var(bool unbiased = true) const { return (*this)().var(unbiased); }
    Series var(std::int64_t dim, bool unbiased = true,
               bool keepdim = false) const {
      return (*this)().var(dim, unbiased, keepdim);
    }
    Series std(bool unbiased = true) const { return (*this)().std(unbiased); }
    Series std(std::int64_t dim, bool unbiased = true,
               bool keepdim = false) const {
      return (*this)().std(dim, unbiased, keepdim);
    }
    Series nanvar() const { return (*this)().nanvar(); }
    Series nanvar(std::int64_t dim, bool keepdim = false) const {
      return (*this)().nanvar(dim, keepdim);
    }
    Series nanstd() const { return (*this)().nanstd(); }
    Series nanstd(std::int64_t dim, bool keepdim = false) const {
      return (*this)().nanstd(dim, keepdim);
    }
    Series skewness() const { return (*this)().skewness(); }
    Series skewness(std::int64_t dim) const { return (*this)().skewness(dim); }
    Series kurtosis() const { return (*this)().kurtosis(); }
    Series kurtosis(std::int64_t dim) const { return (*this)().kurtosis(dim); }
    Series pdf() const { return (*this)().pdf(); }
    Series pdf(double sigma) const { return (*this)().pdf(sigma); }
    Series pdf(double mu, double sigma) const {
      return (*this)().pdf(mu, sigma);
    }
    Series logpdf() const { return (*this)().logpdf(); }
    Series logpdf(double sigma) const { return (*this)().logpdf(sigma); }
    Series logpdf(double mu, double sigma) const {
      return (*this)().logpdf(mu, sigma);
    }
    Series logcdf() const { return (*this)().logcdf(); }
    Series logcdf(double sigma) const { return (*this)().logcdf(sigma); }
    Series logcdf(double mu, double sigma) const {
      return (*this)().logcdf(mu, sigma);
    }
    Series cdf() const { return (*this)().cdf(); }
    Series cdf(double sigma) const { return (*this)().cdf(sigma); }
    Series cdf(double mu, double sigma) const {
      return (*this)().cdf(mu, sigma);
    }
    Series movsum() const { return (*this)().movsum(); }
    Series movsum(std::int64_t window) const {
      return (*this)().movsum(window);
    }
    Series movsum(std::int64_t window, std::int64_t dim) const {
      return (*this)().movsum(window, dim);
    }
    Series movmean() const { return (*this)().movmean(); }
    Series movmean(std::int64_t window) const {
      return (*this)().movmean(window);
    }
    Series movmean(std::int64_t window, std::int64_t dim) const {
      return (*this)().movmean(window, dim);
    }
    Series movmax() const { return (*this)().movmax(); }
    Series movmax(std::int64_t window) const {
      return (*this)().movmax(window);
    }
    Series movmax(std::int64_t window, std::int64_t dim) const {
      return (*this)().movmax(window, dim);
    }
    Series movmin() const { return (*this)().movmin(); }
    Series movmin(std::int64_t window) const {
      return (*this)().movmin(window);
    }
    Series movmin(std::int64_t window, std::int64_t dim) const {
      return (*this)().movmin(window, dim);
    }
    Series movstd() const { return (*this)().movstd(); }
    Series movstd(std::int64_t window) const {
      return (*this)().movstd(window);
    }
    Series movstd(std::int64_t window, std::int64_t dim) const {
      return (*this)().movstd(window, dim);
    }
    Series movvar() const { return (*this)().movvar(); }
    Series movvar(std::int64_t window) const {
      return (*this)().movvar(window);
    }
    Series movvar(std::int64_t window, std::int64_t dim) const {
      return (*this)().movvar(window, dim);
    }
    Series normr() const { return (*this)().normr(); }
    Series normc() const { return (*this)().normc(); }
    Series standardize() const { return (*this)().standardize(); }
    Series standardize(std::int64_t dim) const {
      return (*this)().standardize(dim);
    }
    Series center() const { return (*this)().center(); }
    Series center(std::int64_t dim) const { return (*this)().center(dim); }
    Series pdist2() const { return (*this)().pdist2(); }
    Series pdist2(const Series &other) const { return (*this)().pdist2(other); }
    Series cdist() const { return (*this)().cdist(); }
    Series cdist(const Series &other) const { return (*this)().cdist(other); }

  private:
    Series &owner() const {
      PLUSS_CHECK(owner_ != nullptr, "Series.stat: unbound accessor");
      return *owner_;
    }

    Series *owner_{nullptr};
  };

  // Linear algebra namespace-like proxy.
  // Supports both:
  //   series.linearalg().det()
  //   series.linearalg.det()
  // and peer-bound usage:
  //   series1.linearalg(series2).matmul()
  class SeriesLinearAlgProxy final {
  public:
    using GenEigOptions = pluss::subsp::linearalg::GenEigOptions;
    using GenEigResult = pluss::subsp::linearalg::GenEigResult;

    explicit SeriesLinearAlgProxy(const Series &self) : self_(&self) {}
    SeriesLinearAlgProxy(const Series &self, const Series &other)
        : self_(&self), peers_{other} {}
    SeriesLinearAlgProxy(const Series &self,
                         pluss::msharp::RangeView::Series others)
        : self_(&self), peers_(others.begin(), others.end()) {}

    Series matmul() const {
      return pluss::subsp::linearalg::matmul(self(), peer1());
    }
    Series matmul(const Series &other) const {
      return pluss::subsp::linearalg::matmul(self(), other);
    }
    Series mm() const { return pluss::subsp::linearalg::mm(self(), peer1()); }
    Series mm(const Series &other) const {
      return pluss::subsp::linearalg::mm(self(), other);
    }
    Series mv() const { return pluss::subsp::linearalg::mv(self(), peer1()); }
    Series mv(const Series &other) const {
      return pluss::subsp::linearalg::mv(self(), other);
    }
    Series dot() const { return pluss::subsp::linearalg::dot(self(), peer1()); }
    Series dot(const Series &other) const {
      return pluss::subsp::linearalg::dot(self(), other);
    }
    Series vdot() const {
      return pluss::subsp::linearalg::vdot(self(), peer1());
    }
    Series vdot(const Series &other) const {
      return pluss::subsp::linearalg::vdot(self(), other);
    }
    Series norm() const { return pluss::subsp::linearalg::norm(self()); }

    Series det() const { return pluss::subsp::linearalg::det(self()); }
    Series inverse() const { return pluss::subsp::linearalg::inverse(self()); }
    Series cholesky(bool upper = false) const {
      return pluss::subsp::linearalg::cholesky(self(), upper);
    }
    Series logdet() const { return pluss::subsp::linearalg::logdet(self()); }
    Series slogdet() const { return pluss::subsp::linearalg::slogdet(self()); }
    Series matrix_power(std::int64_t n) const {
      return pluss::subsp::linearalg::matrix_power(self(), n);
    }
    Series matrix_exp() const {
      return pluss::subsp::linearalg::matrix_exp(self());
    }
    Series pinverse(double rcond = 1e-12) const {
      return pluss::subsp::linearalg::pinverse(self(), rcond);
    }
    Series qr_q(bool reduced = true) const {
      return pluss::subsp::linearalg::qr_q(self(), reduced);
    }
    Series qr_r(bool reduced = true) const {
      return pluss::subsp::linearalg::qr_r(self(), reduced);
    }
    Series svd_u(bool full_matrices = true) const {
      return pluss::subsp::linearalg::svd_u(self(), full_matrices);
    }
    Series svd_s(bool full_matrices = true) const {
      return pluss::subsp::linearalg::svd_s(self(), full_matrices);
    }
    Series svd_vh(bool full_matrices = true) const {
      return pluss::subsp::linearalg::svd_vh(self(), full_matrices);
    }
    pluss::subsp::linearalg::SvdDecomposition
    svd(bool full_matrices = true) const;
    pluss::subsp::linearalg::EigenDecomposition eigh(bool upper = false) const;
    template <class ApplyA, class ApplyB, class SolveA>
    GenEigResult
    gen_eigh(ApplyA &&apply_a, ApplyB &&apply_b, SolveA &&solve_a,
             const GenEigOptions &options = {}) const;
    Series eigvalsh(bool upper = false) const {
      return pluss::subsp::linearalg::eigvalsh(self(), upper);
    }
    Series sylv(const Series &b, const Series &c) const {
      return pluss::subsp::linearalg::sylv(self(), b, c);
    }
    Series lyap(
        const Series &q,
        pluss::subsp::linearalg::MatrixEqOptions options = {}) const {
      return pluss::subsp::linearalg::lyap(self(), q, options);
    }
    Series dlyap(
        const Series &q,
        pluss::subsp::linearalg::MatrixEqOptions options = {}) const {
      return pluss::subsp::linearalg::dlyap(self(), q, options);
    }
    Series care(
        const Series &g, const Series &q,
        pluss::subsp::linearalg::MatrixEqOptions options = {}) const {
      return pluss::subsp::linearalg::care(self(), g, q, options);
    }
    Series dare(
        const Series &g, const Series &q,
        pluss::subsp::linearalg::MatrixEqOptions options = {}) const {
      return pluss::subsp::linearalg::dare(self(), g, q, options);
    }

    Series triangular_solve(const Series &b, bool upper = true,
                            bool unitriangular = false) const {
      return pluss::subsp::linearalg::triangular_solve(self(), b, upper,
                                                       unitriangular);
    }
    Series triangular_solve(bool upper = true,
                            bool unitriangular = false) const {
      return pluss::subsp::linearalg::triangular_solve(self(), peer1(), upper,
                                                       unitriangular);
    }
    Series cholesky_solve(const Series &b, bool upper = false) const {
      return pluss::subsp::linearalg::cholesky_solve(self(), b, upper);
    }
    Series cholesky_solve(bool upper = false) const {
      return pluss::subsp::linearalg::cholesky_solve(self(), peer1(), upper);
    }
    Series cholesky_inverse(bool upper = false) const {
      return pluss::subsp::linearalg::cholesky_inverse(self(), upper);
    }
    Series lu_solve(const Series &b) const {
      return pluss::subsp::linearalg::lu_solve(self(), b);
    }
    Series lu_solve() const {
      return pluss::subsp::linearalg::lu_solve(self(), peer1());
    }

  private:
    const Series &self() const {
      PLUSS_CHECK(self_ != nullptr, "Series.linearalg(): null self");
      return *self_;
    }

    const Series &peer1() const {
      PLUSS_CHECK(
          !peers_.empty(),
          "Series.linearalg(...): this operation requires one peer Series");
      return peers_.front();
    }

    const Series *self_{nullptr};
    std::vector<Series> peers_{};
  };

  class SeriesLinearAlgAccessor final {
  public:
    using GenEigOptions = pluss::subsp::linearalg::GenEigOptions;
    using GenEigResult = pluss::subsp::linearalg::GenEigResult;

    SeriesLinearAlgAccessor() = default;
    explicit SeriesLinearAlgAccessor(Series &owner) : owner_(&owner) {}

    void rebind(Series &owner) noexcept { owner_ = &owner; }

    SeriesLinearAlgProxy operator()() const {
      return SeriesLinearAlgProxy(owner());
    }
    SeriesLinearAlgProxy operator()(const Series &other) const {
      return SeriesLinearAlgProxy(owner(), other);
    }
    SeriesLinearAlgProxy
    operator()(pluss::msharp::RangeView::Series others) const {
      return SeriesLinearAlgProxy(owner(), others);
    }

    Series matmul(const Series &other) const { return (*this)().matmul(other); }
    Series mm(const Series &other) const { return (*this)().mm(other); }
    Series mv(const Series &other) const { return (*this)().mv(other); }
    Series dot(const Series &other) const { return (*this)().dot(other); }
    Series vdot(const Series &other) const { return (*this)().vdot(other); }
    Series norm() const { return (*this)().norm(); }
    Series det() const { return (*this)().det(); }
    Series inverse() const { return (*this)().inverse(); }
    Series cholesky(bool upper = false) const {
      return (*this)().cholesky(upper);
    }
    Series logdet() const { return (*this)().logdet(); }
    Series slogdet() const { return (*this)().slogdet(); }
    Series matrix_power(std::int64_t n) const {
      return (*this)().matrix_power(n);
    }
    Series matrix_exp() const { return (*this)().matrix_exp(); }
    Series pinverse(double rcond = 1e-12) const {
      return (*this)().pinverse(rcond);
    }
    Series qr_q(bool reduced = true) const { return (*this)().qr_q(reduced); }
    Series qr_r(bool reduced = true) const { return (*this)().qr_r(reduced); }
    Series svd_u(bool full_matrices = true) const {
      return (*this)().svd_u(full_matrices);
    }
    Series svd_s(bool full_matrices = true) const {
      return (*this)().svd_s(full_matrices);
    }
    Series svd_vh(bool full_matrices = true) const {
      return (*this)().svd_vh(full_matrices);
    }
    pluss::subsp::linearalg::SvdDecomposition
    svd(bool full_matrices = true) const;
    pluss::subsp::linearalg::EigenDecomposition eigh(bool upper = false) const;
    template <class ApplyA, class ApplyB, class SolveA>
    GenEigResult
    gen_eigh(ApplyA &&apply_a, ApplyB &&apply_b, SolveA &&solve_a,
             const GenEigOptions &options = {}) const;
    Series eigvalsh(bool upper = false) const {
      return (*this)().eigvalsh(upper);
    }
    Series sylv(const Series &b, const Series &c) const {
      return (*this)().sylv(b, c);
    }
    Series lyap(
        const Series &q,
        pluss::subsp::linearalg::MatrixEqOptions options = {}) const {
      return (*this)().lyap(q, options);
    }
    Series dlyap(
        const Series &q,
        pluss::subsp::linearalg::MatrixEqOptions options = {}) const {
      return (*this)().dlyap(q, options);
    }
    Series care(
        const Series &g, const Series &q,
        pluss::subsp::linearalg::MatrixEqOptions options = {}) const {
      return (*this)().care(g, q, options);
    }
    Series dare(
        const Series &g, const Series &q,
        pluss::subsp::linearalg::MatrixEqOptions options = {}) const {
      return (*this)().dare(g, q, options);
    }
    Series triangular_solve(const Series &b, bool upper = true,
                            bool unitriangular = false) const {
      return (*this)().triangular_solve(b, upper, unitriangular);
    }
    Series cholesky_solve(const Series &b, bool upper = false) const {
      return (*this)().cholesky_solve(b, upper);
    }
    Series cholesky_inverse(bool upper = false) const {
      return (*this)().cholesky_inverse(upper);
    }
    Series lu_solve(const Series &b) const { return (*this)().lu_solve(b); }

  private:
    Series &owner() const {
      PLUSS_CHECK(owner_ != nullptr, "Series.linearalg: unbound accessor");
      return *owner_;
    }

    Series *owner_{nullptr};
  };

  // Series-native antiderivative accessor:
  // - safe call style: `s.antidiff().trapezoid(x)`, `s.antidiff(x).trapezoid()`
  // - convenience dot style: `s.antidiff.trapezoid(x)`
  // Composite trapezoidal-rule integration along `dim` (torch parity:
  // torch.trapezoid/torch.cumulative_trapezoid -- real torch implements
  // these compositionally too, no dedicated ATen kernel behind them: no
  // `dispatch:` entry in native_functions.yaml, so this mirrors that
  // choice rather than shortcutting it). Built entirely from diff/slice/
  // mul/add/sum/cumsum above, so no new native kernel or backward-manifest
  // entry is needed, and both are differentiable for free through the ops
  // they compose -- see OpsInternal.cpp for the implementation.
  // `pluss::subsp::math::integ::trapezoid`/`cumulative_trapezoid` in
  // PlussMa (PlussMa/include/pluss/subsp/math/integ.h) forward to this
  // accessor rather than duplicate the formula.
  //
  // Named `antidiff` because it is the reverse of `Series::diff()` (the
  // discrete forward-difference method): both operate on an
  // already-materialized sequence of numbers, with no reference to any
  // generating function. This is deliberately NOT a home for continuous
  // (Riemann/Lebesgue) integration of an arbitrary callable -- that needs
  // to re-invoke a function at points it chooses itself (see quadra's
  // 1-D/N-D overloads and aleatoria in PlussMa's `pluss::subsp::math::integ`
  // namespace), which a single already-computed Series value structurally
  // cannot provide.
  class SeriesAntidiffProxy final {
  public:
    explicit SeriesAntidiffProxy(const Series &self) : self_(&self) {}
    SeriesAntidiffProxy(const Series &self, const Series &other)
        : self_(&self), peers_{other} {}

    // Explicit-argument forms -- unambiguous regardless of how the proxy
    // was constructed.
    Series trapezoid(const Series &x, std::int64_t dim = -1) const;
    Series trapezoid(double dx, std::int64_t dim = -1) const;
    // Bound-or-default form: uses the peer x bound via `antidiff(x)` if
    // present, otherwise dx = 1.0 -- mirrors how stat/linearalg's own
    // peer-bound proxies resolve peer1() when bound.
    Series trapezoid(std::int64_t dim = -1) const {
      return peers_.empty() ? trapezoid(1.0, dim) : trapezoid(peer1(), dim);
    }

    Series cumulative_trapezoid(const Series &x, std::int64_t dim = -1) const;
    Series cumulative_trapezoid(double dx, std::int64_t dim = -1) const;
    Series cumulative_trapezoid(std::int64_t dim = -1) const {
      return peers_.empty() ? cumulative_trapezoid(1.0, dim)
                            : cumulative_trapezoid(peer1(), dim);
    }

    // Stochastic integrals of already-sampled values. The receiver holds the
    // integrand sampled at every grid point of the path (n samples along
    // `dim`, or n - 1 for `ito`, which only needs the left endpoints); the
    // peer given to `antidiff(w)` is the integrator path `w` (n samples).
    //   f.antidiff(w).ito()        sum f_i (w_{i+1} - w_i)         Ito
    //   f.antidiff(w).strat()      sum (f_i + f_{i+1})/2 dw_i      Stratonovich
    //   f.antidiff(w).theta(th)    (1 - th) f_i + th f_{i+1}       0 / 1/2 / 1
    // The sums only mean what their names say when f_i was computed from the
    // path up to sample i (an adapted integrand); a Series cannot verify
    // that. PlussMa's `subsp::math::sde` checks it before forwarding here.
    // Without a peer, `w.antidiff().quadratic_variation()` is sum dw^2 and
    // `x.antidiff(y).covariation()` is sum dx dy.
    Series ito(std::int64_t dim = -1) const;
    Series ito_cumulative(std::int64_t dim = -1) const;
    Series strat(std::int64_t dim = -1) const;
    Series strat_cumulative(std::int64_t dim = -1) const;
    Series theta(double theta, std::int64_t dim = -1) const;
    Series quadratic_variation(std::int64_t dim = -1) const;
    Series quadratic_variation_cumulative(std::int64_t dim = -1) const;
    Series covariation(std::int64_t dim = -1) const;
    Series covariation_cumulative(std::int64_t dim = -1) const;

  private:
    const Series &self() const {
      PLUSS_CHECK(self_ != nullptr, "Series.antidiff(): null self");
      return *self_;
    }
    const Series &peer1() const {
      PLUSS_CHECK(!peers_.empty(),
                  "Series.antidiff(...): this operation requires one peer "
                  "Series (the x coordinates)");
      return peers_.front();
    }

    const Series *self_{nullptr};
    std::vector<Series> peers_{};
  };

  class SeriesAntidiffAccessor final {
  public:
    SeriesAntidiffAccessor() = default;
    explicit SeriesAntidiffAccessor(Series &owner) : owner_(&owner) {}

    void rebind(Series &owner) noexcept { owner_ = &owner; }

    SeriesAntidiffProxy operator()() const {
      return SeriesAntidiffProxy(owner());
    }
    SeriesAntidiffProxy operator()(const Series &x) const {
      return SeriesAntidiffProxy(owner(), x);
    }

    Series trapezoid(const Series &x, std::int64_t dim = -1) const {
      return (*this)().trapezoid(x, dim);
    }
    Series trapezoid(double dx, std::int64_t dim = -1) const {
      return (*this)().trapezoid(dx, dim);
    }
    Series trapezoid(std::int64_t dim = -1) const {
      return (*this)().trapezoid(dim);
    }
    Series cumulative_trapezoid(const Series &x, std::int64_t dim = -1) const {
      return (*this)().cumulative_trapezoid(x, dim);
    }
    Series cumulative_trapezoid(double dx, std::int64_t dim = -1) const {
      return (*this)().cumulative_trapezoid(dx, dim);
    }
    Series cumulative_trapezoid(std::int64_t dim = -1) const {
      return (*this)().cumulative_trapezoid(dim);
    }
    Series quadratic_variation(std::int64_t dim = -1) const {
      return (*this)().quadratic_variation(dim);
    }
    Series quadratic_variation_cumulative(std::int64_t dim = -1) const {
      return (*this)().quadratic_variation_cumulative(dim);
    }

  private:
    Series &owner() const {
      PLUSS_CHECK(owner_ != nullptr, "Series.antidiff: unbound accessor");
      return *owner_;
    }

    Series *owner_{nullptr};
  };

  // Callable calculus on the receiver as the independent variable. Both
  // directions live here (`diff` and `antidiff`) and share one set of options:
  //   x.calc.diff(fn)                  first derivative of fn at x
  //   x.calc.diff(fn, 3)               third derivative
  //   x.calc({.order = 2, .lower = a, .initial = {f0, f1}}).antidiff(fn)
  // The implementation stays centralized in subsp::math::diff (no second tape).
  class SeriesCalcAccessor final {
  public:
    enum class Rule : std::uint8_t {
      Trapezoid = 0,
      Simpson = 1,
    };

    // Arguments bound by `x.calc(options)`; each method also accepts the
    // pieces it needs directly, and an explicit argument wins over the option.
    struct CalcOptions {
      // diff: derivative order (0 returns fn(x)). antidiff: number of
      // integrations (at least 1).
      std::int64_t order{1};

      // diff: derivatives are taken elementwise, d fn_i / d x_i, so the result
      // has the shape of x (not the full derivative tensor with one copy of
      // x's axes per order). Requires fn(x) to have the shape of x and to act
      // elementwise; costs one reverse pass per order instead of N^order lanes.
      bool elementwise{false};
      // diff: fail instead of returning zeros when an output does not depend on
      // x.
      bool strict_dependency{false};

      // antidiff: the anchor. F(lower) and its lower derivatives are `initial`.
      std::optional<double> lower{};
      // antidiff: for order n, initial[k] is F^(k)(lower), k = 0..n-1. Empty
      // means all zero (the result then drops the Taylor part of F about
      // lower).
      std::vector<double> initial{};
      Rule rule{Rule::Simpson};
      std::int64_t intervals{256};
      bool adaptive{true};
      double atol{1e-10};
      double rtol{1e-8};
      std::int64_t max_refinements{8};
      // antidiff: throw when adaptive refinement cannot reach the tolerance
      // (otherwise the last estimate is returned).
      bool require_convergence{true};
    };
    // The integration-only subset kept under its original name.
    using AntidiffOptions = CalcOptions;

    // Integrand of a stochastic integral, vectorized over the time axis: `t` is
    // the time of every grid point (broadcastable against `w`) and `w` the
    // integrator's value there. It must act pointwise; since it never sees the
    // integrator's future, a pointwise integrand cannot anticipate.
    using IntegrandFn = std::function<Series(const Series &t, const Series &w)>;
    using ItoFn = std::function<Series(const std::vector<Series> &)>;

    // `x.calc.diff` -- derivatives of a callable at the receiver:
    //   x.calc.diff(fn), x.calc.diff(fn, order)       ordinary derivatives
    //   x.calc.diff.ito(g, mu, sigma, ...)            Ito differential of g(X)
    //   x.calc.diff.strat(g, mu, sigma, ...)   Stratonovich differential
    // See `SeriesItoDifferential` for what the stochastic forms return.
    class DiffAccessor final {
    public:
      DiffAccessor() = default;

      template <class Fn> [[nodiscard]] Series operator()(Fn &&fn) const {
        return Series::calc_diff_(
            owner(),
            std::function<Series(const Series &)>(std::forward<Fn>(fn)),
            opts_.order, opts_);
      }
      template <class Fn>
      [[nodiscard]] Series operator()(Fn &&fn, std::int64_t order) const {
        CalcOptions o = opts_;
        o.order = order;
        return Series::calc_diff_(
            owner(),
            std::function<Series(const Series &)>(std::forward<Fn>(fn)), order,
            o);
      }

      // Ito's formula for g(X) when X is an Ito process dX = mu dt + sigma dW
      // (+ jumps). The receiver is the state X; the points are independent
      // copies, so g must act elementwise. `sigma` has the shape of X (one
      // Brownian factor) or shape(X) + [m] (m factors driving every state).
      [[nodiscard]] SeriesItoDifferential
      ito(const std::function<Series(const Series &)> &g, const Series &mu,
          const Series &sigma) const;
      [[nodiscard]] SeriesItoDifferential
      ito(const std::function<Series(const Series &)> &g, const Series &mu,
          const Series &sigma, const SeriesJumpMeasure &jumps) const;
      // Two states (the receiver and `other`): the multivariate formula,
      // including the covariation term. `ito_product` is g(x, y) = x y.
      [[nodiscard]] SeriesItoDifferential
      ito(const std::function<Series(const Series &, const Series &)> &g,
          const Series &mu, const Series &sigma,
          const SeriesItoState &other) const;
      [[nodiscard]] SeriesItoDifferential
      ito(const std::function<Series(const Series &, const Series &)> &g,
          const Series &mu, const Series &sigma, const SeriesItoState &other,
          const SeriesJumpMeasure &jumps) const;
      [[nodiscard]] SeriesItoDifferential
      ito_product(const Series &mu, const Series &sigma,
                  const SeriesItoState &other) const;
      [[nodiscard]] SeriesItoDifferential
      ito_quotient(const Series &mu, const Series &sigma,
                   const SeriesItoState &other) const;
      // Any number of states; `others` follow the receiver in g's argument
      // list. With `options.time` set, g's first argument is time.
      [[nodiscard]] SeriesItoDifferential
      ito(const ItoFn &g, const Series &mu, const Series &sigma,
          const std::vector<SeriesItoState> &others,
          const SeriesJumpMeasure *jumps,
          const SeriesItoOptions &options) const;
      // g(t, X): time-dependent function of one state.
      [[nodiscard]] SeriesItoDifferential
      ito_at(const Series &time,
             const std::function<Series(const Series &, const Series &)> &g,
             const Series &mu, const Series &sigma) const;

      // Stratonovich differential: the ordinary chain rule, no second-order
      // correction. This is also the right rule for pathwise (Young)
      // integration against fractional Brownian motion with Hurst > 1/2.
      [[nodiscard]] SeriesItoDifferential
      strat(const std::function<Series(const Series &)> &g,
                   const Series &mu, const Series &sigma) const;
      [[nodiscard]] SeriesItoDifferential strat(
          const std::function<Series(const Series &, const Series &)> &g,
          const Series &mu, const Series &sigma,
          const SeriesItoState &other) const;
      [[nodiscard]] SeriesItoDifferential
      strat(const ItoFn &g, const Series &mu, const Series &sigma,
                   const std::vector<SeriesItoState> &others,
                   const SeriesItoOptions &options) const;

      // Marcus (canonical) differential. The process is
      //   dX = mu dt + sigma o dW + h(X-) <> dJ,
      // a Stratonovich diffusion plus jumps that move the state along the
      // flow of dy/du = h(y) z, u in [0, 1] (Marcus 1978/81; Kurtz, Pardoux,
      // Protter 1995). The ordinary chain rule survives: g(X) jumps by exactly
      // g(phi_z(X-)) - g(X-), and the continuous part is the Stratonovich one.
      // `jumps.sizes[0]` holds the jump sizes z_k of the (scalar) driver J.
      // With `compensated`, the jump part is a martingale and the drift gains
      // sum_k nu_k (g(phi_k(x)) - g(x) - g'(x) h(x) z_k).
      [[nodiscard]] SeriesItoDifferential
      marcus(const std::function<Series(const Series &)> &g, const Series &mu,
             const Series &sigma,
             const std::function<Series(const Series &)> &h,
             const SeriesJumpMeasure &jumps) const;
      [[nodiscard]] SeriesItoDifferential
      marcus(const std::function<Series(const Series &)> &g, const Series &mu,
             const Series &sigma,
             const std::function<Series(const Series &)> &h,
             const SeriesJumpMeasure &jumps,
             const SeriesItoOptions &options) const;
      // Where the jump of size z leaves the state: the time-one flow of
      // dy/du = h(y) z started at the receiver (one jump size per point;
      // z broadcasts against the receiver).
      [[nodiscard]] Series
      marcus_flow(const std::function<Series(const Series &)> &h,
                  const Series &z, std::int64_t steps = 32) const;

      // Discrete Malliavin derivative. The receiver holds Brownian
      // increments dW_i along `dim`; F is a functional of them returning one
      // value per remaining index (batch). The result has the shape of the
      // receiver: D_i F = dF/d(dW_i), the derivative of the discretized
      // functional with respect to each increment (the Malliavin derivative
      // D_t F is this value on the cell of dW_i). `malliavin_path` takes the
      // path W (n samples) as the receiver and differentiates with respect
      // to its n - 1 increments, so F sees W rebuilt from them.
      [[nodiscard]] Series
      malliavin(const std::function<Series(const Series &)> &F,
                std::int64_t dim = 0) const;
      [[nodiscard]] Series
      malliavin_path(const std::function<Series(const Series &)> &F,
                     std::int64_t dim = 0) const;
      // Skorokhod integral (the divergence, adjoint of the Malliavin
      // derivative) of a process u = u(dW) that may look into the future:
      //   delta(u) = sum_i u_i dW_i - sum_i (du_i / d dW_i) dt.
      // For an adapted u it reduces to the Ito sum. One reverse pass per
      // increment: use it for the grid sizes of a test or a Greek, not for
      // millions of steps.
      [[nodiscard]] Series
      skorokhod(const std::function<Series(const Series &)> &u, double dt,
                std::int64_t dim = 0) const;

      // Convert drift between conventions for the SAME process
      // dX = mu dt + sigma(X) dW, with sigma(X) a callable of the state (shape
      // of X, or shape(X) + [m]):  mu_stratonovich = mu_ito - 1/2 sigma sigma'
      // and back. Elementwise: every point is a one-dimensional process.
      [[nodiscard]] Series strat_drift(
          const Series &ito_drift,
          const std::function<Series(const Series &)> &sigma) const;
      [[nodiscard]] Series
      ito_drift(const Series &strat_drift,
                const std::function<Series(const Series &)> &sigma) const;

    private:
      friend class SeriesCalcAccessor;
      const Series &owner() const {
        PLUSS_CHECK(owner_ != nullptr, "Series.calc.diff: unbound accessor");
        return *owner_;
      }
      Series *owner_{nullptr};
      CalcOptions opts_{};
    };

    // `x.calc.antidiff` -- integrals ending at the receiver (anchored
    // Riemann integrals), and stochastic integrals along the receiver as a
    // path:
    //   x.calc.antidiff(fn, lower)               Riemann integral lower -> x
    //   w.calc.antidiff.ito(f, t0, t1)           integral of f(t, w) dw, Ito
    //   w.calc.antidiff.strat(f, t0, t1)  same, Stratonovich
    //   w.calc.antidiff.theta(f, th, t0, t1)     th = 0 Ito, 1/2 Strat., 1
    //   backward
    // The path `w` has the time axis `dim` (default 0) and the grid is
    // uniform from t0 to t1, or given explicitly as a time Series.
    class AntidiffAccessor final {
    public:
      AntidiffAccessor() = default;

      template <class Fn> [[nodiscard]] Series operator()(Fn &&fn) const {
        PLUSS_CHECK(opts_.lower.has_value(),
                    "Series.calc.antidiff: no lower bound; pass one or bind "
                    "it with x.calc({.lower = a})");
        return Series::calc_antidiff_(
            owner(),
            std::function<Series(const Series &)>(std::forward<Fn>(fn)),
            *opts_.lower, opts_);
      }
      template <class Fn>
      [[nodiscard]] Series operator()(Fn &&fn, double lower) const {
        return Series::calc_antidiff_(
            owner(),
            std::function<Series(const Series &)>(std::forward<Fn>(fn)), lower,
            opts_);
      }
      template <class Fn>
      [[nodiscard]] Series operator()(Fn &&fn, double lower,
                                      const CalcOptions &options) const {
        return Series::calc_antidiff_(
            owner(),
            std::function<Series(const Series &)>(std::forward<Fn>(fn)), lower,
            options);
      }

      // Total integral over the path (the time axis is reduced).
      [[nodiscard]] Series ito(const IntegrandFn &f, double t0, double t1,
                               std::int64_t dim = 0) const;
      [[nodiscard]] Series ito(const IntegrandFn &f, const Series &times,
                               std::int64_t dim = 0) const;
      [[nodiscard]] Series strat(const IntegrandFn &f, double t0,
                                        double t1, std::int64_t dim = 0) const;
      [[nodiscard]] Series strat(const IntegrandFn &f,
                                        const Series &times,
                                        std::int64_t dim = 0) const;
      // Marcus (canonical) integral of f(t, w) along a path that jumps:
      // Stratonovich averaging on the continuous steps, and over a step
      // flagged as a jump the integral of f along the straight segment,
      //   dw * integral_0^1 f(t, w_i + s dw) ds   (8-point Gauss-Legendre),
      // which is what makes the ordinary chain rule hold for jump paths. The
      // flag is `jump_mask` (true on jump steps, broadcastable against the
      // n - 1 increments) or, in the third form, every step whose |dw|
      // exceeds `jump_threshold`.
      [[nodiscard]] Series marcus(const IntegrandFn &f, const Series &jump_mask,
                                  double t0, double t1,
                                  std::int64_t dim = 0) const;
      [[nodiscard]] Series marcus(const IntegrandFn &f, const Series &jump_mask,
                                  const Series &times,
                                  std::int64_t dim = 0) const;
      [[nodiscard]] Series marcus(const IntegrandFn &f, double jump_threshold,
                                  double t0, double t1,
                                  std::int64_t dim = 0) const;
      [[nodiscard]] Series marcus_cumulative(const IntegrandFn &f,
                                             const Series &jump_mask,
                                             double t0, double t1,
                                             std::int64_t dim = 0) const;
      // Evaluation point inside each step: (1 - theta) f(left) + theta
      // f(right).
      [[nodiscard]] Series theta(const IntegrandFn &f, double theta, double t0,
                                 double t1, std::int64_t dim = 0) const;
      [[nodiscard]] Series theta(const IntegrandFn &f, double theta,
                                 const Series &times,
                                 std::int64_t dim = 0) const;
      // Running integral: same shape as the path, zero at the first sample.
      [[nodiscard]] Series ito_cumulative(const IntegrandFn &f, double t0,
                                          double t1,
                                          std::int64_t dim = 0) const;
      [[nodiscard]] Series ito_cumulative(const IntegrandFn &f,
                                          const Series &times,
                                          std::int64_t dim = 0) const;
      [[nodiscard]] Series strat_cumulative(const IntegrandFn &f,
                                                   double t0, double t1,
                                                   std::int64_t dim = 0) const;
      [[nodiscard]] Series strat_cumulative(const IntegrandFn &f,
                                                   const Series &times,
                                                   std::int64_t dim = 0) const;

    private:
      friend class SeriesCalcAccessor;
      const Series &owner() const {
        PLUSS_CHECK(owner_ != nullptr,
                    "Series.calc.antidiff: unbound accessor");
        return *owner_;
      }
      Series *owner_{nullptr};
      CalcOptions opts_{};
    };

    SeriesCalcAccessor() = default;
    explicit SeriesCalcAccessor(Series &owner) : owner_(&owner) { sync_(); }

    void rebind(Series &owner) noexcept {
      owner_ = &owner;
      sync_();
    }

    // `x.calc()` is the accessor itself; `x.calc(options)` is the same accessor
    // with defaults bound, usable for one expression.
    const SeriesCalcAccessor &operator()() const noexcept { return *this; }
    [[nodiscard]] SeriesCalcAccessor
    operator()(const CalcOptions &options) const {
      SeriesCalcAccessor bound(*owner_ptr());
      bound.options_ = options;
      bound.sync_();
      return bound;
    }
    [[nodiscard]] const CalcOptions &options() const noexcept {
      return options_;
    }

    DiffAccessor diff;
    AntidiffAccessor antidiff;

    // Named derivative shapes. These are constrained views of the same
    // transform engine as diff(), not independent differentiation systems.
    template <class Fn> [[nodiscard]] Series grad(Fn &&fn) const {
      return Series::calc_grad_(
          owner(), std::function<Series(const Series &)>(std::forward<Fn>(fn)),
          options_);
    }
    template <class Fn> [[nodiscard]] Series jac(Fn &&fn) const {
      return Series::calc_jac_(
          owner(), std::function<Series(const Series &)>(std::forward<Fn>(fn)),
          options_);
    }
    template <class Fn> [[nodiscard]] Series hess(Fn &&fn) const {
      return Series::calc_hess_(
          owner(), std::function<Series(const Series &)>(std::forward<Fn>(fn)),
          options_);
    }

  private:
    void sync_() noexcept {
      diff.owner_ = owner_;
      diff.opts_ = options_;
      antidiff.owner_ = owner_;
      antidiff.opts_ = options_;
    }
    Series *owner_ptr() const {
      PLUSS_CHECK(owner_ != nullptr, "Series.calc: unbound accessor");
      return owner_;
    }
    const Series &owner() const { return *owner_ptr(); }

    Series *owner_{nullptr};
    CalcOptions options_{};
  };

  // Factory helpers (Torch-style free-functions live elsewhere; keep these for
  // convenience).
  static Series empty(pluss::msharp::RangeView::Int sizes, ScalarType st);
  static Series empty(pluss::msharp::RangeView::Int sizes, ScalarType st,
                      Platform platform);
  static Series zeros(pluss::msharp::RangeView::Int sizes, ScalarType st);
  static Series zeros(pluss::msharp::RangeView::Int sizes, ScalarType st,
                      Platform platform);
  static Series ones(pluss::msharp::RangeView::Int sizes, ScalarType st);
  static Series ones(pluss::msharp::RangeView::Int sizes, ScalarType st,
                     Platform platform);
  static Series full(pluss::msharp::RangeView::Int sizes, double value,
                     ScalarType st);
  static Series full(pluss::msharp::RangeView::Int sizes, double value,
                     ScalarType st, Platform platform);
  static Series arange(double start, double end, double step, ScalarType dtype,
                       Platform platform);
  static Series arange(double start, double end, double step, ScalarType dtype);
  static Series linspace(double start, double end, std::int64_t steps,
                         ScalarType dtype, Platform platform);
  static Series linspace(double start, double end, std::int64_t steps,
                         ScalarType dtype);
  static Series eye(std::int64_t n, std::int64_t m, ScalarType dtype,
                    Platform platform);
  static Series eye(std::int64_t n, std::int64_t m, ScalarType dtype);
  static Series rand(pluss::msharp::RangeView::Int sizes, ScalarType dtype,
                     std::uint64_t seed, Platform platform);
  static Series rand(pluss::msharp::RangeView::Int sizes, ScalarType dtype,
                     std::uint64_t seed);
  static Series randn(pluss::msharp::RangeView::Int sizes, ScalarType dtype,
                      std::uint64_t seed, Platform platform);
  static Series randn(pluss::msharp::RangeView::Int sizes, ScalarType dtype,
                      std::uint64_t seed);
  static Series from_storage(const Storage &storage,
                             std::int64_t storage_offset,
                             const std::vector<std::int64_t> &sizes,
                             const std::vector<std::int64_t> &strides,
                             ScalarType dtype,
                             Platform platform = Platform::CPU);

  // Core view/cast helpers used across the library.
  Series contiguous() const;
  Series view(pluss::msharp::RangeView::Int new_sizes) const;
  Series reshape(pluss::msharp::RangeView::Int new_sizes) const;
  Series reshape(const std::vector<std::int64_t> &new_sizes) const {
    return reshape(pluss::msharp::RangeView::Int(
        std::span<const std::int64_t>(new_sizes.data(), new_sizes.size())));
  }
  Series reshape(std::initializer_list<std::int64_t> new_sizes) const {
    return reshape(pluss::msharp::RangeView::Int(new_sizes));
  }
  Series permute(pluss::msharp::RangeView::Int dims) const;
  Series to(ScalarType st) const;
  // Torch-like transfer/cast entrypoints.
  // Non-CPU backend support is incremental (MPS currently supports a subset).
  Series to(Platform target) const;
  Series to(Platform target, ScalarType st) const;
  Series clone() const;
  Series transpose(std::int64_t dim0, std::int64_t dim1) const;
  Series unsqueeze(std::int64_t dim) const;
  Series squeeze() const;
  Series squeeze(std::int64_t dim) const;
  Series select(std::int64_t dim, std::int64_t index) const;
  Series slice(std::int64_t dim, std::int64_t start, std::int64_t end,
               std::int64_t step) const;
  Series gather(std::int64_t dim, const Series &index) const;
  Series take(const Series &index) const;
  // `factorial()` (no-arg, elementwise integer factorial) is provided by the
  // generated method surface and routed through the compat op table.
  Series scatter(std::int64_t dim, const Series &index,
                 const Series &src) const;
  Series &scatter_(std::int64_t dim, const Series &index, const Series &src);
  Series scatter_add(std::int64_t dim, const Series &index,
                     const Series &src) const;
  Series &scatter_add_(std::int64_t dim, const Series &index,
                       const Series &src);
  Series scatter_reduce_sum(std::int64_t dim, const Series &index,
                            const Series &src, bool include_self) const;
  Series scatter_reduce_amax(std::int64_t dim, const Series &index,
                             const Series &src, bool include_self) const;
  Series scatter_reduce_amin(std::int64_t dim, const Series &index,
                             const Series &src, bool include_self) const;
  Series scatter_reduce_sum(std::int64_t dim, const Series &index,
                            const Series &src, bool include_self,
                            double initial_scalar) const;
  Series scatter_reduce_amax(std::int64_t dim, const Series &index,
                             const Series &src, bool include_self,
                             double initial_scalar) const;
  Series scatter_reduce_amin(std::int64_t dim, const Series &index,
                             const Series &src, bool include_self,
                             double initial_scalar) const;
  // Internal write kernel behind the autodiff-safe `put_()`/`index_put_()`
  // surface (see Invoke.cpp's put_dispatch_*). Not the public in-place API --
  // that's the generated `Series& put_(...) const` wrapper. Kept as its own
  // non-const, by-value-returning overload so it never shadows the public one.
  Series put_raw_(const Series &index, const Series &source,
                  bool accumulate = false);
  Series nonzero() const;
  Series unique(bool sorted) const;
  Series isin(const Series &test_elements) const;
  Series isin(const Series &test_elements, bool assume_unique) const;
  Series isin(const Series &test_elements, bool assume_unique,
              bool invert) const;
  Series union1d(const Series &other) const;
  Series union1d(const Series &other, bool sorted) const;
  Series unionize(pluss::msharp::RangeView::Of<Series> others,
                  bool sorted = true) const;
  Series intersect1d(const Series &other) const;
  Series intersect1d(const Series &other, bool sorted) const;
  Series intersect1d(const Series &other, bool sorted,
                     bool assume_unique) const;
  Series intersection(pluss::msharp::RangeView::Of<Series> others,
                      bool sorted = true, bool assume_unique = false) const;
  Series setdiff1d(const Series &other) const;
  Series setdiff1d(const Series &other, bool sorted) const;
  Series setdiff1d(const Series &other, bool sorted, bool assume_unique) const;
  Series setxor1d(const Series &other) const;
  Series setxor1d(const Series &other, bool sorted) const;
  Series setxor1d(const Series &other, bool sorted, bool assume_unique) const;
  Series unionize_along(const Series &other, std::int64_t dim) const;
  Series unionize_along(const Series &other, std::int64_t dim,
                        bool sorted) const;
  Series intersection_along(const Series &other, std::int64_t dim) const;
  Series intersection_along(const Series &other, std::int64_t dim,
                            bool sorted) const;
  Series intersection_along(const Series &other, std::int64_t dim, bool sorted,
                            bool assume_unique) const;
  Series setdiff_along(const Series &other, std::int64_t dim) const;
  Series setdiff_along(const Series &other, std::int64_t dim,
                       bool sorted) const;
  Series setdiff_along(const Series &other, std::int64_t dim, bool sorted,
                       bool assume_unique) const;
  Series setxor_along(const Series &other, std::int64_t dim) const;
  Series setxor_along(const Series &other, std::int64_t dim, bool sorted) const;
  Series setxor_along(const Series &other, std::int64_t dim, bool sorted,
                      bool assume_unique) const;
  Series comp(const Series &universe) const;
  Series comp(const Series &universe, bool sorted) const;
  Series comp(const Series &universe, bool sorted, bool assume_unique) const;
  Series cartprod(const Series &other) const;
  Series cartprod(pluss::msharp::RangeView::Of<Series> others) const;
  Series is_prime() const;
  Series prime() const;
  Series factor() const;
  Series bincount(std::int64_t minlength) const;
  Series histc(std::int64_t bins, double min, double max) const;
  Series masked_select(const Series &mask) const;
  Series masked_scatter(const Series &mask, const Series &src) const;
  Series &masked_scatter_(const Series &mask, const Series &src);
  Series bitwise_and(const Series &rhs) const;
  Series bitwise_or(const Series &rhs) const;
  Series bitwise_xor(const Series &rhs) const;
  Series bitwise_not() const;
  Series logical_and(const Series &rhs) const;
  Series logical_or(const Series &rhs) const;
  Series logical_xor(const Series &rhs) const;
  Series logical_not() const;
  Series &logical_not_();
  Series where(const Series &condition, const Series &other) const;
  Series &where_(const Series &condition, const Series &other);
  Series where(const Series &condition, double other) const;
  Series &where_(const Series &condition, double other);
  Series exp() const;
  Series &exp_();
  Series log() const;
  Series &log_();
  Series sqrt() const;
  Series &sqrt_();
  Series sin() const;
  Series &sin_();
  Series cos() const;
  Series &cos_();
  Series tan() const;
  Series &tan_();
  Series tanh() const;
  Series &tanh_();
  Series sinh() const;
  Series &sinh_();
  Series cosh() const;
  Series &cosh_();
  Series asin() const;
  Series &asin_();
  Series acos() const;
  Series &acos_();
  Series atan() const;
  Series &atan_();
  Series asinh() const;
  Series &asinh_();
  Series acosh() const;
  Series &acosh_();
  Series atanh() const;
  Series &atanh_();
  Series erf() const;
  Series &erf_();
  Series erfc() const;
  Series &erfc_();
  Series lgamma() const;
  Series &lgamma_();
  Series log1p() const;
  Series &log1p_();
  Series expm1() const;
  Series &expm1_();
  Series floor() const;
  Series &floor_();
  Series ceil() const;
  Series &ceil_();
  Series round() const;
  Series &round_();
  Series trunc() const;
  Series &trunc_();
  Series reciprocal() const;
  Series &reciprocal_();
  Series sigmoid() const;
  Series &sigmoid_();
  Series relu() const;
  Series &relu_();
  // Track B fused activations: forward-declared here so
  // subsp/math/diff.h's Trace/DualTrace formulas can call them as members
  // (matches at::elu's 3-parameter form; see OpsInternal.h for the shared
  // ops_internal::elu/celu/selu implementation the first three forward to).
  Series elu(double alpha = 1.0, double scale = 1.0,
             double input_scale = 1.0) const;
  Series celu(double alpha = 1.0) const;
  Series selu() const;
  Series hardsigmoid() const;
  Series hardswish() const;
  Series mish() const;
  Series threshold(double threshold_val, double value) const;
  Series rrelu(double lower = 0.125, double upper = 0.3333333333333333,
               bool training = false) const;
  Series isnan() const;
  Series &isnan_();
  Series isinf() const;
  Series &isinf_();
  Series isfinite() const;
  Series &isfinite_();
  Series invalid() const;
  Series invalid_index() const;
  Series mean() const;
  Series mean(std::int64_t dim, bool keepdim) const;
  Series sum(std::int64_t dim, bool keepdim) const;
  Series amax(std::int64_t dim, bool keepdim) const;
  Series amin(std::int64_t dim, bool keepdim) const;
  // Multi-axis reductions (torch's `x.sum({1,2}, keepdim)`): each axis is
  // reduced in turn with keepdim held true internally (mean/sum/amax/amin all
  // commute across separate axes, so this is exact, not an approximation),
  // then the reduced axes are squeezed together if the caller asked for
  // keepdim=false. Differentiable for free -- every step reuses the
  // already-verified single-dim reduction and its reverse rule.
  Series mean(pluss::msharp::RangeView::Int dims, bool keepdim) const;
  Series sum(pluss::msharp::RangeView::Int dims, bool keepdim) const;
  Series amax(pluss::msharp::RangeView::Int dims, bool keepdim) const;
  Series amin(pluss::msharp::RangeView::Int dims, bool keepdim) const;
  // Layer normalization over the trailing `normalized_shape` axes: normalizes
  // each row to zero mean / unit variance (biased) over those axes, then
  // applies the per-feature affine `* weight + bias`. Single fused reverse
  // node (analytic VJP) rather than the ~7-op mean/sub/mul/rsqrt/mul/mul/add
  // composite chain this decomposes to -- see project memory for the formula.
  Series layer_norm(pluss::msharp::RangeView::Int normalized_shape,
                    const Series &weight, const Series &bias,
                    double eps = 1e-5) const;
  Series all(std::int64_t dim, bool keepdim) const;
  Series any(std::int64_t dim, bool keepdim) const;
  Series flatten(std::int64_t start_dim, std::int64_t end_dim) const;
  Series norm(double p) const;
  Series norm(double p, std::int64_t dim, bool keepdim) const;
  Series var(bool unbiased) const;
  Series var(std::int64_t dim, bool unbiased = true,
             bool keepdim = false) const;
  Series std(bool unbiased) const;
  Series std(std::int64_t dim, bool unbiased = true,
             bool keepdim = false) const;
  Series nan_to_num(double nan, double posinf, double neginf) const;
  Series &nan_to_num_(double nan, double posinf, double neginf);
  Series cumsum(std::int64_t dim, bool keepdim) const;
  Series &cumsum_(std::int64_t dim, bool keepdim);
  Series cumprod(std::int64_t dim, bool keepdim) const;
  Series &cumprod_(std::int64_t dim, bool keepdim);
  Series pow(double rhs) const;
  Series &pow_(double rhs);
  Series minimum(double rhs) const;
  Series &minimum_(double rhs);
  Series maximum(double rhs) const;
  Series &maximum_(double rhs);
  Series &add_(double rhs, double alpha);
  Series &add_(const Series &rhs, double alpha);
  Series &add_(const Series &other, double alpha, const Series &out);
  Series &sub_(const Series &rhs, double alpha);
  Series &sub_(const Series &other, double alpha, const Series &out);
  Series &sub_(double rhs, double alpha);
  Series &mul_(const Series &rhs);
  Series &mul_(double rhs);
  Series &div_(const Series &rhs);
  Series &div_(double rhs);
  Series clamp_min(double min) const;
  Series &clamp_min_(double min);
  Series clamp_max(double max) const;
  Series &clamp_max_(double max);
  Series clamp_min(const Series &min) const;
  Series &clamp_min_(const Series &min);
  Series clamp_max(const Series &max) const;
  Series &clamp_max_(const Series &max);
  // Query current backend (Torch-like tensor.device() analog at Platform
  // granularity).
  Platform platform() const noexcept { return SeriesBase::platform(); }
  TernaryCompareBuilder tri() const noexcept;
  Series grouped_count(const Series &group_ids, std::int64_t group_count) const;
  Series grouped_sum(const Series &group_ids, std::int64_t group_count) const;
  Series grouped_mean(const Series &group_ids, std::int64_t group_count) const;
  Series grouped_var(const Series &group_ids, std::int64_t group_count,
                     std::int64_t ddof = 1) const;
  Series grouped_std(const Series &group_ids, std::int64_t group_count,
                     std::int64_t ddof = 1) const;
  Series grouped_min(const Series &group_ids, std::int64_t group_count) const;
  Series grouped_max(const Series &group_ids, std::int64_t group_count) const;
  Series grouped_median(const Series &group_ids,
                        std::int64_t group_count) const;
  Series grouped_mode(const Series &group_ids, std::int64_t group_count) const;

#ifdef PLUSS_ENABLE_TORCH
  ::torch::Tensor tensor() const;
#endif

  void *raw_mutable_data() noexcept {
    return reinterpret_cast<void *>(raw_data());
  }

  // Scalar extraction: only a rank-zero Series has a native scalar form.
  // Returns a native C++ value of the requested type.
  template <class T> T item() const {
    PLUSS_CHECK(dim() == 0,
                "Series::item<T>(): expected a rank-zero Series");
    auto data = detail_materialize_vec_<T>();
    PLUSS_CHECK(data.size() == 1,
                "Series::item<T>(): scalar payload must contain one value");
    return data.front();
  }

  // One-dimensional host extraction. Higher ranks use host<T>(), which
  // preserves rank and shape metadata rather than flattening implicitly.
  template <class T> std::vector<T> pull() const {
    PLUSS_CHECK(dim() == 1,
                "Series::pull<T>(): expected a rank-one Series");
    return detail_materialize_vec_<T>();
  }
  // Dynamic scalar extraction: requires a rank-zero Series.
  // Returns a tagged Scalar when caller doesn't want to spell an explicit C++
  // type. Use item<T>() when exact native type extraction is required.
  Scalar item() const {
    PLUSS_CHECK(dim() == 0,
                "Series::item(): expected a rank-zero Series");
    switch (scalar_type()) {
    case ScalarType::Bool:
      return Scalar(item<bool>(), ScalarType::Bool);
    case ScalarType::UInt8:
      return Scalar(static_cast<std::uint64_t>(item<std::uint8_t>()),
                    ScalarType::UInt8);
    case ScalarType::UInt16:
      return Scalar(static_cast<std::uint64_t>(item<std::uint16_t>()),
                    ScalarType::UInt16);
    case ScalarType::UInt32:
      return Scalar(static_cast<std::uint64_t>(item<std::uint32_t>()),
                    ScalarType::UInt32);
    case ScalarType::UInt64:
      return Scalar(item<std::uint64_t>(), ScalarType::UInt64);
    case ScalarType::Int8:
      return Scalar(static_cast<std::int64_t>(item<std::int8_t>()),
                    ScalarType::Int8);
    case ScalarType::Int16:
      return Scalar(static_cast<std::int64_t>(item<std::int16_t>()),
                    ScalarType::Int16);
    case ScalarType::Int32:
      return Scalar(static_cast<std::int64_t>(item<std::int32_t>()),
                    ScalarType::Int32);
    case ScalarType::Int64:
      return Scalar(item<std::int64_t>(), ScalarType::Int64);
    case ScalarType::Float16:
    case ScalarType::BFloat16:
    case ScalarType::Float8E4M3FN:
    case ScalarType::Float8E5M2:
      return Scalar(to(ScalarType::Float64).item<double>(), scalar_type());
    case ScalarType::Float32:
      return Scalar(static_cast<double>(item<float>()), ScalarType::Float32);
    case ScalarType::Float64:
      return Scalar(item<double>(), ScalarType::Float64);
    case ScalarType::Complex64:
    case ScalarType::Complex128:
      PLUSS_FAIL("Series::item(): complex dtype requires explicit "
                 "item<Complex64>() or item<Complex128>()");
    default:
      PLUSS_FAIL(
          "Series::item(): unsupported dtype for dynamic scalar extraction");
    }
  }

  // Host-export convenience entrypoint.
  // Typed variant:
  // - Scalar series materializes as scalar T
  // - 1-D series materializes as std::vector<T>
  // - N-D series materializes as HostArray<T> (data + sizes/strides)
  template <class T> HostValue<T> host() const {
    auto arr = detail_host_array_impl_<T>();
    if (dim() == 0)
      return HostValue<T>::from_scalar(std::move(arr));
    if (dim() == 1)
      return HostValue<T>::from_vector(std::move(arr));
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
    case ScalarType::Bool:
      return detail_host_any_impl_<bool>(st);
    case ScalarType::UInt8:
      return detail_host_any_impl_<std::uint8_t>(st);
    case ScalarType::UInt16:
      return detail_host_any_impl_<std::uint16_t>(st);
    case ScalarType::UInt32:
      return detail_host_any_impl_<std::uint32_t>(st);
    case ScalarType::UInt64:
      return detail_host_any_impl_<std::uint64_t>(st);
    case ScalarType::Int8:
      return detail_host_any_impl_<std::int8_t>(st);
    case ScalarType::Int16:
      return detail_host_any_impl_<std::int16_t>(st);
    case ScalarType::Int32:
      return detail_host_any_impl_<std::int32_t>(st);
    case ScalarType::Int64:
      return detail_host_any_impl_<std::int64_t>(st);
    case ScalarType::Float16:
      return detail_host_any_impl_<Float16>(st);
    case ScalarType::BFloat16:
      return detail_host_any_impl_<BFloat16>(st);
    case ScalarType::Float32:
      return detail_host_any_impl_<float>(st);
    case ScalarType::Float64:
      return detail_host_any_impl_<double>(st);
    case ScalarType::Float8E4M3FN:
      return detail_host_any_float32_bridge_(st);
    case ScalarType::Float8E5M2:
      return detail_host_any_float32_bridge_(st);
    case ScalarType::Complex64:
      return detail_host_any_impl_<Complex64>(st);
    case ScalarType::Complex128:
      return detail_host_any_impl_<Complex128>(st);
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

  // Native C++ introspection results. Elementwise predicates such as
  // isclose(), isnan(), and isfinite() intentionally continue to return Series.
  bool equal(const Series &rhs) const;
  bool allclose(const Series &rhs, double rtol = 1e-5, double atol = 1e-8,
                bool equal_nan = false) const;
  bool is_leaf() const;
  bool is_nonzero() const;
  bool is_same_size(const Series &rhs) const noexcept;
  bool is_set_to(const Series &rhs) const noexcept;

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
  // These are depended on internally (e.g. norm implementation) and are
  // implemented in Series.cpp / OpsInternal.cpp

  // Variance / std (dim overloads are used internally but not yet codegen'd).
  Series var() const { return var(true); }
  Series std() const { return std(true); }

  // Value/index reductions. Returned as std::pair<values, indices> -- a fixed
  // arity of 2, so a pair rather than a variable-length container.
  std::pair<Series, Series> max_pair(std::int64_t dim,
                                     bool keepdim = false) const;
  std::pair<Series, Series> min_pair(std::int64_t dim,
                                     bool keepdim = false) const;
  std::pair<Series, Series> sort_pair(std::int64_t dim = -1,
                                      bool descending = false,
                                      bool stable = false) const;
  std::pair<Series, Series> topk_pair(std::int64_t k, std::int64_t dim = -1,
                                      bool largest = true,
                                      bool sorted = true) const;

  // Series-valued second arguments for the two-input special functions. The
  // scalar overloads are generated alongside the rest of the surface.
  Series beta(const Series &other) const;
  Series betaln(const Series &other) const;
  Series zeta(const Series &other) const;

  // Decompose each element x into (mantissa, exponent) with
  // x == mantissa * 2**exponent and 0.5 <= |mantissa| < 1 (both 0 when x == 0).
  // Fixed arity of 2, so a pair; the exponent piece is int32 and never carries
  // gradient.
  std::pair<Series, Series> frexp() const;

  // Smallest and largest values, together. Returned as std::pair<min, max> --
  // a fixed arity of 2. The whole-tensor form reduces to two scalars; the
  // dim form reduces that one axis. Both channels are differentiable (each
  // routes its gradient to the position of the extremum, ties shared).
  std::pair<Series, Series> aminmax() const;
  std::pair<Series, Series> aminmax(std::int64_t dim,
                                    bool keepdim = false) const;

  // Bucket the values into `bins` equal-width bins and return
  // std::pair<hist, bin_edges>: `hist` holds the per-bin counts (float64, or a
  // probability density when `density` is true) and `bin_edges` the `bins + 1`
  // boundaries. With no range the bins span the finite [min, max] of the data.
  // Not differentiable.
  std::pair<Series, Series> histogram(std::int64_t bins = 100,
                                      bool density = false) const;
  std::pair<Series, Series> histogram(std::int64_t bins, double lo, double hi,
                                      bool density = false) const;

  // Split family: partition one axis into a variable-length list of pieces
  // (each a view). Returns std::vector<Series> because the piece count is a
  // runtime value -- the same choice torch makes for chunk/split/unbind.
  std::vector<Series> chunk(std::int64_t chunks, std::int64_t dim = 0) const;
  std::vector<Series> split(std::int64_t split_size,
                            std::int64_t dim = 0) const;
  std::vector<Series> split(pluss::msharp::RangeView::Int split_sizes,
                            std::int64_t dim = 0) const;
  std::vector<Series>
  split_with_sizes(pluss::msharp::RangeView::Int split_sizes,
                   std::int64_t dim = 0) const;
  std::vector<Series> unbind(std::int64_t dim = 0) const;

  // Contraction family (torch parity: torch.tensordot / torch.einsum).
  // Composites over diagonal/sum/permute/reshape/bmm, so reverse-mode
  // autograd flows through the existing verified reverse rules.
  //
  // tensordot(other, n): contract the last `n` axes of this with the first
  // `n` axes of `other` (n = 0 is the outer product). The explicit overload
  // contracts `dims_self[i]` with `dims_other[i]` (negative axes allowed);
  // result axes are this's free axes followed by other's free axes. A
  // contracted pair whose sizes differ is allowed only when one of them is 1
  // (that axis is summed first), as in torch.
  Series tensordot(const Series &other, std::int64_t dims = 2) const;
  Series tensordot(const Series &other,
                   const std::vector<std::int64_t> &dims_self,
                   const std::vector<std::int64_t> &dims_other) const;
  // einsum(equation, operands): Einstein summation with torch's grammar --
  // lower/upper-case subscripts, implicit mode (no "->": output is the labels
  // seen once, ellipsis axes first, then alphabetical), explicit mode, "..."
  // broadcasting, repeated subscripts within one operand (diagonal), and
  // size-1 broadcasting of a shared label. Operands are contracted pairwise
  // left to right, each pair as one batched matmul.
  static Series einsum(std::string_view equation,
                       const std::vector<Series> &operands);
  std::vector<Series> tensor_split(std::int64_t sections,
                                   std::int64_t dim = 0) const;
  std::vector<Series> tensor_split(pluss::msharp::RangeView::Int indices,
                                   std::int64_t dim = 0) const;
  std::vector<Series> hsplit(std::int64_t sections) const;
  std::vector<Series> hsplit(pluss::msharp::RangeView::Int indices) const;
  std::vector<Series> vsplit(std::int64_t sections) const;
  std::vector<Series> vsplit(pluss::msharp::RangeView::Int indices) const;
  std::vector<Series> dsplit(std::int64_t sections) const;
  std::vector<Series> dsplit(pluss::msharp::RangeView::Int indices) const;
  // `unsafe_*` are the same operation here (every piece is already a view).
  std::vector<Series> unsafe_chunk(std::int64_t chunks,
                                   std::int64_t dim = 0) const;
  std::vector<Series> unsafe_split(std::int64_t split_size,
                                   std::int64_t dim = 0) const;
  std::vector<Series> unsafe_split(pluss::msharp::RangeView::Int split_sizes,
                                   std::int64_t dim = 0) const;
  std::vector<Series>
  unsafe_split_with_sizes(pluss::msharp::RangeView::Int split_sizes,
                          std::int64_t dim = 0) const;

  // Pluss autodiff entrypoint. This is a named wrapper over `_backward(...)`
  // so the public surface stays Pluss-native.
  void reverse() const {
    (void)pluss::codem::_compat::invoke("_backward", *this);
  }
  void reverse(bool keep_graph, bool accumulate_sensitivity = false,
               bool track_higher_order = false) const {
    (void)pluss::codem::_compat::invoke(
        "_backward", *this,
        {keep_graph, accumulate_sensitivity, track_higher_order});
  }
  void reverse(const Series &sensitivity_output) const {
    (void)pluss::codem::_compat::invoke("_backward", *this,
                                        {sensitivity_output});
  }
  void reverse(const Series &sensitivity_output, bool keep_graph,
               bool accumulate_sensitivity = false,
               bool track_higher_order = false) const {
    (void)pluss::codem::_compat::invoke("_backward", *this,
                                        {sensitivity_output, keep_graph,
                                         accumulate_sensitivity,
                                         track_higher_order});
  }

  bool tracks_slope() const {
    return pluss::codem::_compat::invoke("tracks_slope", *this).item<bool>();
  }
  bool has_sensitivity() const {
    return pluss::codem::_compat::invoke("has_sensitivity", *this).item<bool>();
  }
  Series sensitivity() const {
    return pluss::codem::_compat::invoke("sensitivity", *this);
  }
  void clear() const {
    (void)pluss::codem::_compat::invoke_("clear_sensitivity", *this);
  }
  std::int64_t storage_version() const {
    return pluss::codem::_compat::invoke("_version", *this)
        .item<std::int64_t>();
  }

  Series count(bool count_true = true) const {
    PLUSS_CHECK(defined(), "Series::count: undefined Series");
    const auto target_platform = platform();
    const auto cpu = is_cpu() ? *this : to(Platform::CPU);

    std::int64_t total = 0;
    if (cpu.scalar_type() == ScalarType::Bool) {
      const auto values = cpu.host<bool>();
      total = static_cast<std::int64_t>(
          std::count(values.begin(), values.end(), count_true));
    } else {
      PLUSS_CHECK(count_true,
                  "Series::count(false) is only valid for Bool Series");
      total = cpu.numel();
    }

    auto out_cpu = Series::empty({1}, ScalarType::Int64, Platform::CPU);
    reinterpret_cast<std::int64_t *>(out_cpu.raw_data())[0] = total;
    return target_platform == Platform::CPU ? out_cpu
                                            : out_cpu.to(target_platform);
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

  static bool extract_consumes_dim_(const ExtractSpec_ &s) {
    return s.kind == ExtractSpec::Kind::Slice ||
           s.kind == ExtractSpec::Kind::ScalarIndex ||
           s.kind == ExtractSpec::Kind::AdvancedIndex;
  }

  static std::vector<ExtractSpec_>
  normalize_extract_specs_(const std::vector<ExtractSpec_> &specs,
                           std::int64_t rank) {
    std::int64_t consumed_without_ellipsis = 0;
    std::int64_t ellipsis_count = 0;
    for (const auto &s : specs) {
      if (s.kind == ExtractSpec::Kind::Ellipsis) {
        ++ellipsis_count;
      } else if (extract_consumes_dim_(s)) {
        ++consumed_without_ellipsis;
      }
    }
    PLUSS_CHECK(ellipsis_count <= 1,
                "Series::extract: at most one ellipsis is allowed");

    std::vector<ExtractSpec_> out;
    out.reserve(specs.size() +
                static_cast<std::size_t>(std::max<std::int64_t>(0, rank)));

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
    PLUSS_CHECK(fill >= 0, "Series::extract: ellipsis cannot compensate for "
                           "too many indexing specs");
    for (const auto &s : specs) {
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

  static std::int64_t normalize_scalar_index_(std::int64_t i,
                                              std::int64_t dim_size) {
    if (i < 0)
      i += dim_size;
    PLUSS_CHECK(i >= 0 && i < dim_size,
                "Series::extract: scalar index out of range");
    return i;
  }

  static ExtractSliceBounds_
  normalize_slice_bounds_(const pluss::Slicing::Slice &s,
                          std::int64_t dim_size) {
    PLUSS_CHECK(dim_size >= 0, "Series::extract: invalid dimension size");
    PLUSS_CHECK(s.step != 0, "Series::extract: step cannot be 0");
    ExtractSliceBounds_ b;
    b.step = s.step;
    if (s.step > 0) {
      b.start = (s.start == pluss::Slicing::Slice::kUnbounded)
                    ? 0
                    : (s.start < 0 ? s.start + dim_size : s.start);
      b.stop = (s.stop == pluss::Slicing::Slice::kUnbounded)
                   ? dim_size
                   : (s.stop < 0 ? s.stop + dim_size : s.stop);
      b.start = std::clamp<std::int64_t>(b.start, 0, dim_size);
      b.stop = std::clamp<std::int64_t>(b.stop, 0, dim_size);
    } else {
      b.start = (s.start == pluss::Slicing::Slice::kUnbounded)
                    ? (dim_size - 1)
                    : (s.start < 0 ? s.start + dim_size : s.start);
      b.stop = (s.stop == pluss::Slicing::Slice::kUnbounded)
                   ? -1
                   : (s.stop < 0 ? s.stop + dim_size : s.stop);
      b.start = std::clamp<std::int64_t>(b.start, -1, dim_size - 1);
      b.stop = std::clamp<std::int64_t>(b.stop, -1, dim_size - 1);
    }
    return b;
  }

  static bool is_identity_slice_(const ExtractSliceBounds_ &b,
                                 std::int64_t dim_size) {
    return b.step == 1 && b.start == 0 && b.stop == dim_size;
  }

  static std::vector<std::int64_t>
  materialize_dim_indices_(const ExtractSpec_ &spec, std::int64_t dim_size) {
    PLUSS_CHECK(dim_size >= 0, "Series::extract: invalid dimension size");

    if (spec.kind == ExtractSpec::Kind::ScalarIndex) {
      return {normalize_scalar_index_(spec.scalar_index, dim_size)};
    }

    if (spec.kind == ExtractSpec::Kind::Slice) {
      const auto b = normalize_slice_bounds_(spec.slice, dim_size);

      std::vector<std::int64_t> out;
      if ((b.step > 0 && b.start >= b.stop) ||
          (b.step < 0 && b.start <= b.stop)) {
        return out;
      }
      for (std::int64_t i = b.start; (b.step > 0) ? (i < b.stop) : (i > b.stop);
           i += b.step) {
        out.push_back(i);
      }
      return out;
    }

    PLUSS_CHECK(spec.kind == ExtractSpec::Kind::AdvancedIndex,
                "Series::extract: invalid internal spec kind");
    PLUSS_CHECK(spec.advanced_impl,
                "Series::extract: advanced index Series is undefined");
    Series idx_series = pluss::series::undefined();
    idx_series.impl_ = spec.advanced_impl;

    const Series idx_cpu = idx_series.to(Platform::CPU);
    if (idx_cpu.scalar_type() == ScalarType::Bool) {
      PLUSS_CHECK(idx_cpu.dim() == 1 && idx_cpu.size(0) == dim_size,
                  "Series::extract: boolean advanced index must be 1D and "
                  "match the indexed dimension size");
      const auto mask = idx_cpu.host<bool>();
      std::vector<std::int64_t> out;
      out.reserve(mask.size());
      for (std::int64_t i = 0; i < static_cast<std::int64_t>(mask.size());
           ++i) {
        if (mask[static_cast<std::size_t>(i)])
          out.push_back(i);
      }
      return out;
    }

    const auto st = idx_cpu.scalar_type();
    const bool is_integral =
        st == ScalarType::UInt8 || st == ScalarType::UInt16 ||
        st == ScalarType::UInt32 || st == ScalarType::UInt64 ||
        st == ScalarType::Int8 || st == ScalarType::Int16 ||
        st == ScalarType::Int32 || st == ScalarType::Int64;
    PLUSS_CHECK(
        is_integral,
        "Series::extract: advanced index Series must be bool or integer dtype");
    const auto flat = idx_cpu.to(ScalarType::Int64)
                          .reshape({idx_cpu.numel()})
                          .host<std::int64_t>();
    std::vector<std::int64_t> out;
    out.reserve(flat.size());
    for (std::int64_t i : flat) {
      if (i < 0)
        i += dim_size;
      PLUSS_CHECK(i >= 0 && i < dim_size,
                  "Series::extract: advanced index out of range");
      out.push_back(i);
    }
    return out;
  }

  static Series make_index_series_on_(std::int64_t dim_size,
                                      const ExtractSpec_ &spec,
                                      Platform platform) {
    if (spec.kind == ExtractSpec::Kind::Slice) {
      const auto b = normalize_slice_bounds_(spec.slice, dim_size);
      const bool empty =
          (b.step > 0) ? (b.start >= b.stop) : (b.start <= b.stop);
      if (empty) {
        Series out = Series::empty({0}, ScalarType::Int64, Platform::CPU);
        return platform == Platform::CPU ? out
                                         : out.to(platform, ScalarType::Int64);
      }
      Series idx = Series::arange(
          static_cast<double>(b.start), static_cast<double>(b.stop),
          static_cast<double>(b.step), ScalarType::Int64);
      if (platform != Platform::CPU) {
        idx = idx.to(platform, ScalarType::Int64);
      }
      return idx;
    }

    const auto idx = materialize_dim_indices_(spec, dim_size);
    Series out = Series::empty({static_cast<std::int64_t>(idx.size())},
                               ScalarType::Int64, Platform::CPU);
    auto *p = reinterpret_cast<std::int64_t *>(out.raw_data());
    for (std::int64_t i = 0; i < static_cast<std::int64_t>(idx.size()); ++i) {
      p[i] = idx[static_cast<std::size_t>(i)];
    }
    return platform == Platform::CPU ? out
                                     : out.to(platform, ScalarType::Int64);
  }

  Series extract_from_specs_(const std::vector<ExtractSpec_> &specs) const {
    PLUSS_CHECK(defined(), "Series::extract: undefined Series");
    const auto normalized = normalize_extract_specs_(specs, dim());

    Series out = *this;
    std::int64_t dim_cursor = 0;
    for (const auto &spec : normalized) {
      if (spec.kind == ExtractSpec::Kind::NewAxis) {
        out = out.unsqueeze(dim_cursor);
        ++dim_cursor;
        continue;
      }
      PLUSS_CHECK(dim_cursor < out.dim(),
                  "Series::extract: internal dimension cursor overflow");
      const std::int64_t n = out.size(dim_cursor);
      if (spec.kind == ExtractSpec::Kind::ScalarIndex) {
        out = out.select(dim_cursor,
                         normalize_scalar_index_(spec.scalar_index, n));
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
      out =
          pluss::codem::_compat::invoke("index_select", out, {dim_cursor, idx});
      ++dim_cursor;
    }
    return out;
  }

  static bool
  can_traverse_as_view_(const std::vector<ExtractSpec_> &normalized) {
    for (const auto &spec : normalized) {
      if (spec.kind == ExtractSpec::Kind::AdvancedIndex)
        return false;
      if (spec.kind == ExtractSpec::Kind::Slice && spec.slice.step <= 0)
        return false;
    }
    return true;
  }

  Series
  traverse_slice_from_specs_(const std::vector<ExtractSpec_> &specs) const {
    PLUSS_CHECK(defined(), "Series::traverse_slice: undefined Series");
    const auto normalized = normalize_extract_specs_(specs, dim());

    if (!can_traverse_as_view_(normalized)) {
      PLUSS_FAIL("Series::traverse_slice: selector is not view-representable; "
                 "use extract() or extract_put_() for materializing paths");
    }

    Series out = *this;
    std::int64_t dim_cursor = 0;
    for (const auto &spec : normalized) {
      if (spec.kind == ExtractSpec::Kind::NewAxis) {
        out = out.unsqueeze(dim_cursor);
        ++dim_cursor;
        continue;
      }

      PLUSS_CHECK(dim_cursor < out.dim(),
                  "Series::traverse_slice: internal dimension cursor overflow");
      const std::int64_t n = out.size(dim_cursor);

      if (spec.kind == ExtractSpec::Kind::ScalarIndex) {
        out = out.select(dim_cursor,
                         normalize_scalar_index_(spec.scalar_index, n));
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

  Series &extract_put_from_specs_(const std::vector<ExtractSpec_> &specs,
                                  const Series &values) {
    PLUSS_CHECK(defined(), "Series::extract_put_: undefined Series");
    const auto normalized = normalize_extract_specs_(specs, dim());
    std::vector<Series> indices(static_cast<std::size_t>(dim()),
                                pluss::series::undefined());
    std::vector<Series> defined_raw;
    std::vector<std::int64_t> defined_dims;
    defined_raw.reserve(static_cast<std::size_t>(dim()));
    defined_dims.reserve(static_cast<std::size_t>(dim()));

    std::int64_t selected_numel = 1;
    std::int64_t src_dim = 0;
    for (const auto &spec : normalized) {
      if (spec.kind == ExtractSpec::Kind::NewAxis)
        continue;
      PLUSS_CHECK(src_dim < dim(),
                  "Series::extract_put_: too many consuming specs");
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
    PLUSS_CHECK(src_dim == dim(), "Series::extract_put_: normalized specs did "
                                  "not consume all source dimensions");

    if (selected_numel == 0)
      return *this;

    if (!defined_raw.empty()) {
      const std::size_t k = defined_raw.size();
      for (std::size_t i = 0; i < k; ++i) {
        std::vector<std::int64_t> shape(k, 1);
        shape[i] = defined_raw[i].numel();
        indices[static_cast<std::size_t>(defined_dims[i])] =
            defined_raw[i].reshape(
                std::span<const std::int64_t>(shape.data(), shape.size()));
      }
    }

    Series src = values;
    if (src.scalar_type() != scalar_type() || src.platform() != platform()) {
      src = src.to(platform(), scalar_type());
    }
    PLUSS_CHECK(src.numel() == 1 || src.numel() == selected_numel,
                "Series::extract_put_: source must be scalar or match selected "
                "element count");
    if (src.numel() != 1) {
      src = src.reshape({selected_numel});
    }

    pluss::codem::_compat::invoke_(
        "index_put_", *this,
        {pluss::msharp::RangeView::Of<Series>(indices), src, false});
    return *this;
  }

  Series extract_put_copy_from_specs_(const std::vector<ExtractSpec_> &specs,
                                      const Series &values) const {
    PLUSS_CHECK(defined(), "Series::extract_put: undefined Series");
    const auto normalized = normalize_extract_specs_(specs, dim());
    std::vector<Series> indices(static_cast<std::size_t>(dim()),
                                pluss::series::undefined());
    std::vector<Series> defined_raw;
    std::vector<std::int64_t> defined_dims;
    defined_raw.reserve(static_cast<std::size_t>(dim()));
    defined_dims.reserve(static_cast<std::size_t>(dim()));

    std::int64_t selected_numel = 1;
    std::int64_t src_dim = 0;
    for (const auto &spec : normalized) {
      if (spec.kind == ExtractSpec::Kind::NewAxis)
        continue;
      PLUSS_CHECK(src_dim < dim(),
                  "Series::extract_put: too many consuming specs");
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
    PLUSS_CHECK(src_dim == dim(), "Series::extract_put: normalized specs did "
                                  "not consume all source dimensions");

    Series out = clone();
    if (selected_numel == 0)
      return out;

    if (!defined_raw.empty()) {
      const std::size_t k = defined_raw.size();
      for (std::size_t i = 0; i < k; ++i) {
        std::vector<std::int64_t> shape(k, 1);
        shape[i] = defined_raw[i].numel();
        indices[static_cast<std::size_t>(defined_dims[i])] =
            defined_raw[i].reshape(
                std::span<const std::int64_t>(shape.data(), shape.size()));
      }
    }

    Series src = values;
    if (src.scalar_type() != scalar_type() || src.platform() != platform()) {
      src = src.to(platform(), scalar_type());
    }
    PLUSS_CHECK(src.numel() == 1 || src.numel() == selected_numel,
                "Series::extract_put: source must be scalar or match selected "
                "element count");
    if (src.numel() != 1) {
      src = src.reshape({selected_numel});
    }

    return pluss::codem::_compat::invoke(
        "index_put", out,
        {pluss::msharp::RangeView::Of<Series>(indices), src, false});
  }

public:
  // Traversal/view surface. Selector must be view-representable.
  Series traverse_slice(pluss::msharp::RangeView::Of<ExtractSpec> specs) const {
    return traverse_slice_from_specs_({specs.begin(), specs.end()});
  }

  Series &traverse_put_(pluss::msharp::RangeView::Of<ExtractSpec> specs,
                        const Series &values) {
    const std::vector<ExtractSpec> parsed(specs.begin(), specs.end());
    const auto normalized = normalize_extract_specs_(parsed, dim());
    if (!can_traverse_as_view_(normalized)) {
      PLUSS_FAIL("Series::traverse_put_: selector is not view-representable; "
                 "use extract() or extract_put_() for materializing paths");
    }
    Series view = traverse_slice_from_specs_(parsed);
    if (values.numel() == view.numel()) {
      Series src = values;
      if (std::vector<std::int64_t>(src.sizes().begin(), src.sizes().end()) !=
          std::vector<std::int64_t>(view.sizes().begin(), view.sizes().end())) {
        src = src.reshape(view.sizes());
      }
      (void)pluss::codem::_compat::invoke_("copy_", view, {src});
      return *this;
    }
    return extract_put_from_specs_(parsed, values);
  }

  Series traverse_put(pluss::msharp::RangeView::Of<ExtractSpec> specs,
                      const Series &values) const {
    const std::vector<ExtractSpec> parsed(specs.begin(), specs.end());
    const auto normalized = normalize_extract_specs_(parsed, dim());
    if (!can_traverse_as_view_(normalized)) {
      PLUSS_FAIL("Series::traverse_put: selector is not view-representable; "
                 "use extract() or extract_put() for materializing paths");
    }
    Series out = clone();
    Series view = out.traverse_slice_from_specs_(parsed);
    if (values.numel() == view.numel()) {
      Series src = values;
      if (std::vector<std::int64_t>(src.sizes().begin(), src.sizes().end()) !=
          std::vector<std::int64_t>(view.sizes().begin(), view.sizes().end())) {
        src = src.reshape(view.sizes());
      }
      (void)pluss::codem::_compat::invoke_("copy_", view, {src});
      return out;
    }
    return extract_put_copy_from_specs_(parsed, values);
  }

  // Dynamic extract surface: accepts a dynamic list of specs.
  Series extract(pluss::msharp::RangeView::Of<ExtractSpec> specs) const {
    return extract_from_specs_({specs.begin(), specs.end()});
  }

  Series extract(ExtractSpec a0) const {
    const std::array<ExtractSpec, 1> specs{a0};
    return extract(pluss::msharp::RangeView::Of<ExtractSpec>(specs));
  }

  Series extract(ExtractSpec a0, ExtractSpec a1) const {
    const std::array<ExtractSpec, 2> specs{a0, a1};
    return extract(pluss::msharp::RangeView::Of<ExtractSpec>(specs));
  }

  Series extract(ExtractSpec a0, ExtractSpec a1, ExtractSpec a2) const {
    const std::array<ExtractSpec, 3> specs{a0, a1, a2};
    return extract(pluss::msharp::RangeView::Of<ExtractSpec>(specs));
  }

  Series extract(ExtractSpec a0, ExtractSpec a1, ExtractSpec a2,
                 ExtractSpec a3) const {
    const std::array<ExtractSpec, 4> specs{a0, a1, a2, a3};
    return extract(pluss::msharp::RangeView::Of<ExtractSpec>(specs));
  }

  Series &extract_put_(pluss::msharp::RangeView::Of<ExtractSpec> specs,
                       const Series &values) {
    return extract_put_from_specs_({specs.begin(), specs.end()}, values);
  }

  Series extract_put(pluss::msharp::RangeView::Of<ExtractSpec> specs,
                     const Series &values) const {
    return extract_put_copy_from_specs_({specs.begin(), specs.end()}, values);
  }

  // PlussQit naming deviations (method-first aliases).
  Series pile(pluss::msharp::RangeView::Of<Series> others,
              std::int64_t dim = 0) const;
  Series unite(pluss::msharp::RangeView::Of<Series> others,
               std::int64_t dim = 0, bool strict_dtype = true) const;

  // Canonical indexing entrypoint. This accepts heterogeneous index specs
  // (advanced-index Series, scalar indices, SliceAll/SliceRange, NewAxis,
  // Ellipsis). Implementation lives in OpsInternal.cpp.
  Series index(pluss::msharp::RangeView::Of<ExtractSpec> specs) const;
  Series index_put(pluss::msharp::RangeView::Of<Series> indices,
                   const Series &values, bool accumulate) const {
    return pluss::codem::_compat::invoke("index_put", *this,
                                         {indices, values, accumulate});
  }
  Series index_put(std::initializer_list<Series> indices, const Series &values,
                   bool accumulate) const {
    return index_put(pluss::msharp::RangeView::Of<Series>(indices), values,
                     accumulate);
  }
  Series index_put(std::int64_t index, const Series &values) const {
    Series out = *this;
    out.index_put_(index, values, false);
    return out;
  }
  Series &index_put_(pluss::msharp::RangeView::Of<Series> indices,
                     const Series &values, bool accumulate) const {
    return pluss::codem::_compat::invoke_("index_put_", *this,
                                          {indices, values, accumulate});
  }
  Series &index_put_(std::initializer_list<Series> indices,
                     const Series &values, bool accumulate) const {
    return index_put_(pluss::msharp::RangeView::Of<Series>(indices), values,
                      accumulate);
  }

  // Torch-like scalar index convenience: x.index_put_(0, value)
  // This overload is intentionally 1D-only for explicitness.
  Series &index_put_(std::int64_t index, const Series &values,
                     bool accumulate = false) const {
    PLUSS_CHECK(dim() == 1,
                "Series::index_put_(int64, value): only valid for 1D Series; "
                "for higher dimensions use index list or extract_put_");
    PLUSS_CHECK(
        values.numel() == 1,
        "Series::index_put_(int64, value): value must be scalar (numel==1)");
    const std::int64_t i = normalize_scalar_index_(index, size(0));
    Series idx = Series::empty({1}, ScalarType::Int64, Platform::CPU);
    reinterpret_cast<std::int64_t *>(idx.raw_data())[0] = i;
    if (platform() != Platform::CPU) {
      idx = idx.to(platform(), ScalarType::Int64);
    }
    const std::array<Series, 1> idx_arr{{idx}};
    return index_put_(pluss::msharp::RangeView::Of<Series>(idx_arr), values,
                      accumulate);
  }

  // Slice-spec index convenience for common 1D/2D scalar assignment forms:
  // - 1D: x.index_put_({i}, value)
  // - 2D: x.index_put_({SliceAll(), j}, col_values)
  // For richer N-D slicing/writes use extract_put_ directly.
  Series &index_put_(std::initializer_list<ExtractSpec> specs,
                     const Series &values, bool accumulate = false) const {
    PLUSS_CHECK(!accumulate,
                "Series::index_put_(specs, value, accumulate): accumulate=true "
                "is unsupported for slice-spec indexing; use Series index list "
                "overload instead");
    const std::vector<ExtractSpec> parsed(specs.begin(), specs.end());
    PLUSS_CHECK(!parsed.empty(),
                "Series::index_put_(specs, value): specs cannot be empty");
    PLUSS_CHECK(
        parsed.size() <= 2,
        "Series::index_put_(specs, value): only 1D and 2D convenience specs "
        "are supported; use extract_put_ for higher-dimensional writes");

    if (parsed.size() == 1) {
      PLUSS_CHECK(dim() == 1,
                  "Series::index_put_({i}, value): target must be 1D");
      PLUSS_CHECK(
          parsed[0].kind == ExtractSpec::Kind::ScalarIndex,
          "Series::index_put_({i}, value): spec must be a scalar index");
      PLUSS_CHECK(
          values.numel() == 1,
          "Series::index_put_({i}, value): value must be scalar (numel==1)");
      return const_cast<Series *>(this)->extract_put_(parsed, values);
    }

    PLUSS_CHECK(
        dim() == 2,
        "Series::index_put_({SliceAll(), j}, value): target must be 2D");
    PLUSS_CHECK(parsed[0].kind == ExtractSpec::Kind::Slice &&
                    parsed[0].slice.is_all(),
                "Series::index_put_({SliceAll(), j}, value): first spec must "
                "be SliceAll()");
    PLUSS_CHECK(parsed[1].kind == ExtractSpec::Kind::ScalarIndex,
                "Series::index_put_({SliceAll(), j}, value): second spec must "
                "be a scalar column index");
    PLUSS_CHECK(values.dim() == 1 && values.numel() == size(0),
                "Series::index_put_({SliceAll(), j}, value): value must be 1D "
                "with length equal to size(0)");
    return const_cast<Series *>(this)->extract_put_(parsed, values);
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

  // Accessors are subordinate Series surfaces, not independent value types.
  // These aliases are the canonical public spellings; the longer class names
  // remain as implementation identities to preserve ABI compatibility.
  using StatAccessor = SeriesStatAccessor;
  using LinearAlgAccessor = SeriesLinearAlgAccessor;
  using AntidiffAccessor = SeriesAntidiffAccessor;
  using CalcAccessor = SeriesCalcAccessor;
  // Public spellings of the stochastic-calculus value types used by
  // `calc.diff.ito(...)` and friends.
  using ItoState = SeriesItoState;
  using JumpMeasure = SeriesJumpMeasure;
  using ItoOptions = SeriesItoOptions;
  using ItoDifferential = SeriesItoDifferential;
  using TraverseAccessor = SeriesTraverseAccessor;

  // Statistical accessor:
  // - safe call style: `s.stat().mean()`
  // - convenience dot style: `s.stat.mean()`
  StatAccessor stat;
  // Linear algebra accessor:
  // - safe call style: `s.linearalg().det()`
  // - convenience dot style: `s.linearalg.det()`
  LinearAlgAccessor linearalg;
  // Antiderivative accessor -- reverses `diff()` (discrete forward
  // differencing on an already-materialized sequence), not autodiff:
  // - safe call style: `s.antidiff().trapezoid(x)`, `s.antidiff(x).trapezoid()`
  // - convenience dot style: `s.antidiff.trapezoid(x)`
  AntidiffAccessor antidiff;
  // Callable calculus. `diff` differentiates a function at this Series;
  // `antidiff` evaluates an anchored repeated integral ending at this Series.
  // - safe call style: `x.calc().diff(fn, order)`
  // - convenience dot style: `x.calc.diff(fn, order)`
  // - bound arguments: `x.calc({.order = 2, .lower = a}).antidiff(fn)`
  CalcAccessor calc;
  // Traverse/query accessor:
  // - dot style: `s.traverse.find_all(mask)`
  // - with explicit `on` selector: `s.traverse(specs).find_ge(...)`
  TraverseAccessor traverse;

  // An empty handle, equivalent to pluss::series::undefined(). Defined Series
  // values remain constructible only through the public factory functions.
  Series()
      : SeriesBase(), stat(*this), linearalg(*this), antidiff(*this),
        calc(*this), traverse(*this) {}
  Series(const Series &other)
      : SeriesBase(other), stat(*this), linearalg(*this), antidiff(*this),
        calc(*this), traverse(*this) {}
  Series(Series &&other) noexcept
      : SeriesBase(std::move(other)), stat(*this), linearalg(*this),
        antidiff(*this), calc(*this), traverse(*this) {}
  Series &operator=(const Series &other) {
    if (this != &other) {
      SeriesBase::operator=(other);
    }
    stat.rebind(*this);
    linearalg.rebind(*this);
    antidiff.rebind(*this);
    calc.rebind(*this);
    traverse.rebind(*this);
    return *this;
  }
  Series &operator=(Series &&other) noexcept {
    if (this != &other) {
      SeriesBase::operator=(std::move(other));
    }
    stat.rebind(*this);
    linearalg.rebind(*this);
    antidiff.rebind(*this);
    calc.rebind(*this);
    traverse.rebind(*this);
    return *this;
  }

private:
  static Series calc_diff_(const Series &input,
                           std::function<Series(const Series &)> callback,
                           std::int64_t order,
                           const SeriesCalcAccessor::CalcOptions &options);
  static Series calc_grad_(const Series &input,
                           std::function<Series(const Series &)> callback,
                           const SeriesCalcAccessor::CalcOptions &options);
  static Series calc_jac_(const Series &input,
                          std::function<Series(const Series &)> callback,
                          const SeriesCalcAccessor::CalcOptions &options);
  static Series calc_hess_(const Series &input,
                           std::function<Series(const Series &)> callback,
                           const SeriesCalcAccessor::CalcOptions &options);
  static Series
  calc_antidiff_(const Series &upper,
                 std::function<Series(const Series &)> callback, double lower,
                 const SeriesCalcAccessor::AntidiffOptions &options);
  static SeriesItoDifferential calc_ito_(
      const Series &x, const SeriesCalcAccessor::ItoFn &g, const Series &mu,
      const Series &sigma, const std::vector<SeriesItoState> &others,
      const SeriesJumpMeasure *jumps, const SeriesItoOptions &options,
      bool ito_convention,
      const std::function<std::vector<Series>(const std::vector<Series> &)>
          *marcus_h = nullptr);
  static Series calc_malliavin_(const Series &receiver,
                                const std::function<Series(const Series &)> &F,
                                std::int64_t dim, bool from_path);
  static Series calc_skorokhod_(const Series &increments,
                                const std::function<Series(const Series &)> &u,
                                double dt, std::int64_t dim);
  static Series calc_marcus_integral_(
      const Series &path, const SeriesCalcAccessor::IntegrandFn &f,
      const Series &times, const Series &jump_mask, std::int64_t dim,
      bool cumulative);
  static Series array_stochastic_integral_(const Series &values,
                                           const Series &path, double theta,
                                           std::int64_t dim, bool cumulative);
  static Series
  calc_convention_drift_(const Series &x, const Series &drift,
                         const std::function<Series(const Series &)> &sigma,
                         double sign);
  static Series calc_stochastic_integral_(
      const Series &path, const SeriesCalcAccessor::IntegrandFn &f,
      const Series &times, double theta, std::int64_t dim, bool cumulative);

  template <class> struct detail_false_ : std::false_type {};

  template <class T> HostArray<T> detail_host_array_impl_() const {
    HostArray<T> out;
    out.sizes.assign(sizes().begin(), sizes().end());
    out.strides.assign(strides().begin(), strides().end());
    out.contiguous = is_contiguous();
    out.data = detail_materialize_vec_<T>();
    return out;
  }

  template <class T> HostAny detail_host_any_impl_(ScalarType st) const {
    auto arr = detail_host_array_impl_<T>();
    if (dim() == 0) {
      PLUSS_CHECK(!arr.data.empty(), "Series::host(): scalar payload is empty");
      return HostAny::from_scalar(HostScalarAny{arr.data.front()}, st);
    }
    if (dim() == 1) {
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
    if (cpu_f32.dim() == 0) {
      PLUSS_CHECK(!arr.data.empty(), "Series::host(): scalar payload is empty");
      return HostAny::from_scalar(HostScalarAny{arr.data.front()},
                                  original_dtype);
    }
    if (cpu_f32.dim() == 1) {
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

  template <class T> std::vector<T> detail_materialize_vec_() const {
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
        const auto *base = reinterpret_cast<const bool *>(raw_data());
        for (std::size_t i = 0; i < out.size(); ++i) {
          out[i] = base[i];
        }
      } else {
        std::memcpy(out.data(), raw_data(), out.size() * sizeof(T));
      }
      return out;
    }

    const auto *base = reinterpret_cast<const T *>(raw_data());
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

  template <class T> static ScalarType detail_scalar_type_for_() {
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
      static_assert(detail_false_<U>::value,
                    "Series::host<T>: unsupported element type");
    }
  }

  // Internal-only defined-state ctor. End users create defined Series values
  // through factories; public default construction creates no payload.
  explicit Series(pluss::msharp::intrusive_ptr<SeriesImpl> impl)
      : SeriesBase(std::move(impl)), stat(*this), linearalg(*this),
        antidiff(*this), calc(*this), traverse(*this) {}

  // Internal construction hook used by ops_internal implementations.
  friend struct ops_internal::SeriesFactoryAccess;
};

// An Ito process observed at a point: the state and its coefficients,
//   dX = mu dt + sigma dW.
// `sigma` has the shape of `x` (a single Brownian factor) or shape(x) + [m]
// (m factors; every state in one calculation shares the same m factors, which
// is what makes covariations between states meaningful).
struct SeriesItoState {
  Series x;
  Series mu;
  Series sigma;
};

// The jump part of a Levy-driven process as an atomic measure. Atom k is a
// jump of size sizes[j][k] in state j that occurs at rate intensity[k] per unit
// time. A compound Poisson process with rate lambda and jump law p_k is
// intensity[k] = lambda p_k; a Levy measure with a density is represented by
// quadrature atoms and weights. Each sizes[j] is [K] or [K] + shape(x),
// intensity is [K] or [K] + shape(x).
//
// compensated = false: X = ... + (sum of the jumps).
// compensated = true:  X = ... + (sum of the jumps) - (sum_k intensity_k
//                      size_k) dt, i.e. the jump part is a martingale.
struct SeriesJumpMeasure {
  std::vector<Series> sizes;
  Series intensity;
  bool compensated{false};
};

struct SeriesItoOptions {
  // g(t, x...) instead of g(x...): the first argument g receives is time.
  Series time;
  // Marcus jumps: Runge-Kutta steps used to follow the flow of a jump.
  std::int64_t flow_steps{32};
};

// Differential of g(X): dg = drift dt + diffusion . dW + jumps.
struct SeriesItoDifferential {
  Series value;      // g at the point
  Series drift;      // coefficient of dt (in the representation of the input)
  Series diffusion;  // coefficients of the m Brownian factors, shape(x) + [m]
  Series jump_sizes; // g(x + z_k) - g(x), shape [K] + shape(x); undefined
                     // without jumps
  Series jump_intensity; // [K] or [K] + shape(x); undefined without jumps
  bool jumps_compensated{false};
};

inline ExtractSpec::ExtractSpec(const Series &idx) : kind(Kind::AdvancedIndex) {
  PLUSS_CHECK(idx.defined(),
              "Series::extract: advanced index Series is undefined");
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
