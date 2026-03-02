#pragma once

#include "pluss/msharp/msharp.h"
#include "CodeM/CodeM.h"
#include "CodeM/core/Printing.h"
#include <cstdint>
#include <functional>
#include <span>

// Optional: LibTorch interop (enable PLUSS_ENABLE_TORCH)
#ifdef PLUSS_ENABLE_TORCH
#include <pluss/codem/torch_interop.h>
#endif

namespace pluss::codem {

template <class T>
concept SeriesScalar =
    std::is_arithmetic_v<std::remove_cv_t<T>> && !std::is_same_v<std::remove_cv_t<T>, bool>;

inline Series make_scalar_like(const Series& like, double value) {
  return Series::full({}, value, like.scalar_type()).to(like.platform());
}

enum class TernaryCompareOp : std::uint8_t { Eq, Ne, Lt, Le, Gt, Ge };

using BlobDeleter = std::function<void(void*)>;

CODEM_API bool mps_backend_available() noexcept;
Series from_buffer(void* data,
                   std::span<const std::int64_t> sizes,
                   std::span<const std::int64_t> strides,
                   BlobDeleter deleter,
                   ScalarType dtype);

struct TernaryCompareBuilder {
  const Series& first;
};

struct TernaryCompareChain {
  const Series& first;
  const Series& second;
  TernaryCompareOp first_op;
};

inline TernaryCompareBuilder Series::tri() const noexcept { return TernaryCompareBuilder{*this}; }

inline Series ternary_compare_eval(const Series& a, const Series& b, TernaryCompareOp op) {
  switch (op) {
    case TernaryCompareOp::Eq: return a.eq(b);
    case TernaryCompareOp::Ne: return a.ne(b);
    case TernaryCompareOp::Lt: return a.lt(b);
    case TernaryCompareOp::Le: return a.le(b);
    case TernaryCompareOp::Gt: return a.gt(b);
    case TernaryCompareOp::Ge: return a.ge(b);
    default: return a.eq(b);
  }
}

inline TernaryCompareChain operator==(TernaryCompareBuilder lhs, const Series& rhs) noexcept {
  return TernaryCompareChain{lhs.first, rhs, TernaryCompareOp::Eq};
}
inline TernaryCompareChain operator!=(TernaryCompareBuilder lhs, const Series& rhs) noexcept {
  return TernaryCompareChain{lhs.first, rhs, TernaryCompareOp::Ne};
}
inline TernaryCompareChain operator<(TernaryCompareBuilder lhs, const Series& rhs) noexcept {
  return TernaryCompareChain{lhs.first, rhs, TernaryCompareOp::Lt};
}
inline TernaryCompareChain operator<=(TernaryCompareBuilder lhs, const Series& rhs) noexcept {
  return TernaryCompareChain{lhs.first, rhs, TernaryCompareOp::Le};
}
inline TernaryCompareChain operator>(TernaryCompareBuilder lhs, const Series& rhs) noexcept {
  return TernaryCompareChain{lhs.first, rhs, TernaryCompareOp::Gt};
}
inline TernaryCompareChain operator>=(TernaryCompareBuilder lhs, const Series& rhs) noexcept {
  return TernaryCompareChain{lhs.first, rhs, TernaryCompareOp::Ge};
}

inline Series ternary_compare_finish(const TernaryCompareChain& chain,
                                     const Series& third,
                                     TernaryCompareOp second_op) {
  const Series first_mask = ternary_compare_eval(chain.first, chain.second, chain.first_op);
  const Series second_mask = ternary_compare_eval(chain.second, third, second_op);
  return first_mask.logical_and(second_mask);
}

inline Series operator==(const TernaryCompareChain& chain, const Series& rhs) {
  return ternary_compare_finish(chain, rhs, TernaryCompareOp::Eq);
}
inline Series operator!=(const TernaryCompareChain& chain, const Series& rhs) {
  return ternary_compare_finish(chain, rhs, TernaryCompareOp::Ne);
}
inline Series operator<(const TernaryCompareChain& chain, const Series& rhs) {
  return ternary_compare_finish(chain, rhs, TernaryCompareOp::Lt);
}
inline Series operator<=(const TernaryCompareChain& chain, const Series& rhs) {
  return ternary_compare_finish(chain, rhs, TernaryCompareOp::Le);
}
inline Series operator>(const TernaryCompareChain& chain, const Series& rhs) {
  return ternary_compare_finish(chain, rhs, TernaryCompareOp::Gt);
}
inline Series operator>=(const TernaryCompareChain& chain, const Series& rhs) {
  return ternary_compare_finish(chain, rhs, TernaryCompareOp::Ge);
}

template <SeriesScalar T>
inline Series operator+(T lhs, const Series& rhs) {
  return make_scalar_like(rhs, static_cast<double>(lhs)).add(rhs);
}

template <SeriesScalar T>
inline Series operator-(T lhs, const Series& rhs) {
  return make_scalar_like(rhs, static_cast<double>(lhs)).sub(rhs);
}

template <SeriesScalar T>
inline Series operator*(T lhs, const Series& rhs) {
  return make_scalar_like(rhs, static_cast<double>(lhs)).mul(rhs);
}

template <SeriesScalar T>
inline Series operator/(T lhs, const Series& rhs) {
  return make_scalar_like(rhs, static_cast<double>(lhs)).div(rhs);
}

template <SeriesScalar T>
inline Series operator==(T lhs, const Series& rhs) {
  return make_scalar_like(rhs, static_cast<double>(lhs)).eq(rhs);
}

template <SeriesScalar T>
inline Series operator!=(T lhs, const Series& rhs) {
  return make_scalar_like(rhs, static_cast<double>(lhs)).ne(rhs);
}

template <SeriesScalar T>
inline Series operator<(T lhs, const Series& rhs) {
  return make_scalar_like(rhs, static_cast<double>(lhs)).lt(rhs);
}

template <SeriesScalar T>
inline Series operator<=(T lhs, const Series& rhs) {
  return make_scalar_like(rhs, static_cast<double>(lhs)).le(rhs);
}

template <SeriesScalar T>
inline Series operator>(T lhs, const Series& rhs) {
  return make_scalar_like(rhs, static_cast<double>(lhs)).gt(rhs);
}

template <SeriesScalar T>
inline Series operator>=(T lhs, const Series& rhs) {
  return make_scalar_like(rhs, static_cast<double>(lhs)).ge(rhs);
}

inline bool platform_available(Platform p) noexcept {
  switch (p) {
    case Platform::CPU:
      return true;
    case Platform::CUDA:
      return false;
    case Platform::MPS:
      return mps_backend_available();
    default:
      return false;
  }
}

} // namespace pluss::codem

namespace pluss {
  using Series = codem::Series;
  using Config = codem::Config;
  using ScalarType = codem::ScalarType;
  using Platform = codem::Platform;
  inline constexpr Platform pCPU = Platform::CPU;
  inline constexpr Platform pMPS = Platform::MPS;
  using BlobDeleter = codem::BlobDeleter;
  using PrintOptions = codem::PrintOptions;

  using codem::to_string;
  using codem::print_series;
  using codem::platform_available;
  using codem::operator<<;

  inline Series index(const Series& self, const std::vector<Series>& indices) {
    return self.index(indices);
  }

  inline Series index_put(const Series& self,
                          const std::vector<Series>& indices,
                          const Series& values,
                          bool accumulate=false) {
    return self.index_put(indices, values, accumulate);
  }

} // namespace pluss

namespace pluss::mps {

inline bool is_ready() noexcept {
  return codem::platform_available(codem::Platform::MPS);
}

inline bool is_usable() noexcept {
  return is_ready();
}

} // namespace pluss::mps
