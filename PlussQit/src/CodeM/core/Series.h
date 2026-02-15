#pragma once
#include <cstdint>
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
#include "CodeM/core/Scalar.h"
#include "MSharp/SnapPtr.h"


namespace pluss::codem {

// Forward declaration for tensor memory/layout format used by generated APIs.
// Full definition should come from the appropriate header if/when needed.
enum class MemoryFormat;

// Torch-like integer array reference type used by generated APIs.
// We represent it as a span of 64-bit ints.

// User-facing Series (Torch-like Tensor).
// User-facing Series (Torch-like Tensor).
class Series : public SeriesBase {
public:
  using SeriesBase::SeriesBase;

  // Factory helpers (Torch-style free-functions live elsewhere; keep these for convenience).
  static Series empty(std::span<const std::int64_t> sizes, ScalarType st);
  static Series zeros(std::span<const std::int64_t> sizes, ScalarType st);
  static Series ones(std::span<const std::int64_t> sizes, ScalarType st);
  static Series full(std::span<const std::int64_t> sizes, double value, ScalarType st);
  static Series make_from_doubles(const std::vector<double>& flat, std::span<const std::int64_t> sizes, ScalarType st);

  // Core view/cast helpers used across the library.
  Series contiguous() const;
  Series to(ScalarType st) const;
  Series clone() const;

  void* raw_mutable_data() noexcept { return reinterpret_cast<void*>(raw_data()); }

  // Expanded Torch-aligned method surface (auto-generated declarations).
  #include "CodeM/core/SeriesMethods.inc"

  // ---- C++ convenience operators (Torch-like) ----
  // These are thin wrappers around the generated method surface.
  // NOTE: We only provide Series-vs-Series operators here. Scalar operators
  // can be added later once the scalar type used by CodeM is finalized.

  // Out-of-place arithmetic
  Series operator+(const Series& other) const { return add(other); }
  Series operator-(const Series& other) const { return sub(other); }
  Series operator*(const Series& other) const { return mul(other); }
  Series operator/(const Series& other) const { return div(other); }

  // In-place arithmetic
  Series& operator+=(const Series& other) { add_(other); return *this; }
  Series& operator-=(const Series& other) { sub_(other); return *this; }
  Series& operator*=(const Series& other) { mul_(other); return *this; }
  Series& operator/=(const Series& other) { div_(other); return *this; }

  // In-place bitwise
  Series& operator&=(const Series& other) { bitwise_and_(other); return *this; }
  Series& operator|=(const Series& other) { bitwise_or_(other); return *this; }
  Series& operator^=(const Series& other) { bitwise_xor_(other); return *this; }

  // ---- Core reductions / a small torch-compatible surface ----
  // These are depended on internally (e.g. norm implementation) and are implemented
  // in Series.cpp / OpsInternal.cpp

  // Variance / std (dim overloads are used internally but not yet codegen'd).
  Series var(std::int64_t dim, bool unbiased=true, bool keepdim=false) const;
  Series std(std::int64_t dim, bool unbiased=true, bool keepdim=false) const;

  // Norm overloads used throughout the codebase.
  Series norm(double p, std::int64_t dim, bool keepdim=false) const;

  // PlussQit naming deviations (method-first aliases).
  Series pile(const std::vector<Series>& others, std::int64_t dim=0) const;
  Series unite(const std::vector<Series>& others, std::int64_t dim=0) const;
};


} // namespace pluss::codem
