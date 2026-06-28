#pragma once
#include "CodeM/core/SeriesSlim.h"

namespace pluss::codem::native::cpu {

  void scatter_add(const Series& self, std::int64_t dim, const Series& index, const Series& src, Series& out);

  enum class ScatterReduce { Sum, AMax, AMin };

  // scatter_reduce knobs:
  // - include_self: if false, out is initialized from `initial` (if provided) or the identity for the reduction.
  // - initial: optional scalar initial value (applied to all out elements before reductions).
  void scatter_reduce(const Series& self, std::int64_t dim, const Series& index, const Series& src,
                      ScatterReduce reduce, Series& out,
                      bool include_self = true,
                      bool has_initial = false,
                      double initial_scalar = 0.0);

} // namespace pluss::codem::native::cpu
