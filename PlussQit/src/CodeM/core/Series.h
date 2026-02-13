#pragma once
#include <cstdint>
#include <span>
#include <vector>
#include <type_traits>
#include <iosfwd>

#include "CodeM/core/Errors.h"

#include "CodeM/core/SeriesBase.h"
#include "CodeM/core/DType.h"
#include "CodeM/core/Storage.h"
#include "CodeM/core/ScalarType.h"
#include "MSharp/SnapPtr.h"

namespace pluss::codem {

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

  // ---- Core reductions / a small torch-compatible surface ----
  // These are depended on internally (e.g. norm implementation) and are implemented
  // in Series.cpp / OpsInternal.cpp.
  Series sum() const;
  Series sum(std::int64_t dim, bool keepdim=false) const;
  Series mean() const;
  Series mean(std::int64_t dim, bool keepdim=false) const;
  Series prod() const;
  Series prod(std::int64_t dim, bool keepdim=false) const;
  Series amax() const;
  Series amax(std::int64_t dim, bool keepdim=false) const;
  Series amin() const;
  Series amin(std::int64_t dim, bool keepdim=false) const;
  Series argmax(std::int64_t dim=-1, bool keepdim=false) const;
  Series argmin(std::int64_t dim=-1, bool keepdim=false) const;

  // Variance / std (dim overloads are used internally but not yet codegen'd).
  Series var(std::int64_t dim, bool unbiased=true, bool keepdim=false) const;
  Series std(std::int64_t dim, bool unbiased=true, bool keepdim=false) const;

  // Norm overloads used throughout the codebase.
  Series norm(double p, std::int64_t dim, bool keepdim=false) const;

  // PlussQit naming deviations (method-first aliases).
  // - pile: stack alias
  // - unite: cat alias
  Series pile(const std::vector<Series>& others, std::int64_t dim=0) const;
  Series unite(const std::vector<Series>& others, std::int64_t dim=0) const;
};


} // namespace pluss::codem
