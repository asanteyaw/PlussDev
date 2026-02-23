#pragma once

#include "pluss/msharp/msharp.h"
#include "CodeM/CodeM.h"
#include "CodeM/core/Printing.h"
#include <cstdint>

// Optional: LibTorch interop (enable PLUSS_ENABLE_TORCH)
#ifdef PLUSS_ENABLE_TORCH
#include <pluss/codem/torch_interop.h>
#endif

namespace pluss::codem {

enum class TernaryCompareOp : std::uint8_t { Eq, Ne, Lt, Le, Gt, Ge };

CODEM_API bool mps_backend_available() noexcept;

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

} // namespace pluss

namespace pluss::mps {

inline bool is_usable() noexcept {
  return codem::platform_available(codem::Platform::MPS);
}

} // namespace pluss::mps
