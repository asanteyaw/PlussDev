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
#include "MSharp/IntrusivePtr.h"
#include "pluss/export.h"


namespace pluss::codem {

namespace ops_internal {
struct SeriesFactoryAccess;
} // namespace ops_internal


// Forward declaration for tensor memory/layout format used by generated APIs.
// Full definition should come from the appropriate header if/when needed.
enum class MemoryFormat;

// User-facing Series (Torch-like Tensor).
// User-facing Series (Torch-like Tensor).
class CODEM_API Series : public SeriesBase {
public:
  Series() = default;

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
  Series var(std::int64_t dim, bool unbiased=true, bool keepdim=false) const;
  Series std(std::int64_t dim, bool unbiased=true, bool keepdim=false) const;

  // Norm overloads used throughout the codebase.
  Series norm(double p, std::int64_t dim, bool keepdim=false) const;

  // PlussQit naming deviations (method-first aliases).
  Series pile(const std::vector<Series>& others, std::int64_t dim=0) const;
  Series unite(const std::vector<Series>& others, std::int64_t dim=0) const;

private:
  // Internal-only ctor: Series objects are created via factories and internal
  // kernels (ops_internal), not by end users.
  explicit Series(pluss::msharp::intrusive_ptr<SeriesImpl> impl)
      : SeriesBase(std::move(impl)) {}

  // Internal construction hook used by ops_internal implementations.
  friend struct ops_internal::SeriesFactoryAccess;
};


} // namespace pluss::codem
