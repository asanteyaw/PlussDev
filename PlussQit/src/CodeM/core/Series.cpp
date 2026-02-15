#include "CodeM/core/Series.h"
#include "CodeM/core/Errors.h"
#include "CodeM/core/SeriesImpl.h"
#include "CodeM/core/SeriesIterator.h"
#include "CodeM/core/Half.h"
#include "CodeM/native/cpu/ArithmeticKernels.h"
#include "CodeM/native/cpu/AdvancedIndexingKernels.h"
#include "CodeM/native/cpu/BitwiseKernels.h"
#include "CodeM/core/ViewInvariants.h"
#include "CodeM/core/Config.h"
#include <ostream>
#include "CodeM/native/cpu/ScatterAddKernels.h"
#include "CodeM/native/cpu/GatherScatterKernels.h"
#include "CodeM/native/cpu/ConcatStackKernels.h"
#include "CodeM/native/cpu/UnaryMathKernels.h"
#include "CodeM/native/cpu/UnaryMoreKernels.h"
#include "CodeM/native/cpu/ReduceKernels.h"
#include "CodeM/native/cpu/WhereClampKernels.h"
#include "CodeM/native/cpu/CumKernels.h"
#include "CodeM/native/cpu/KernelTable.h"
#include "CodeM/core/TypePromotion.h"
#include "CodeM/core/OpsInternal.h"
#include "CodeM/core/Complex.h"

#include <algorithm>
#include <array>
#include <vector>
#include <cmath>
#include <cstring>
#include <cstring>

namespace pluss::codem {

  // ---- Extra convenience methods declared in Series.h (Torch-inspired, CPU-only for now) ----

  Series Series::var(std::int64_t dim, bool unbiased, bool keepdim) const {
    // Torch-style variance along a dimension.
    // We compute: mean((x - mean(x))^2) and optionally apply Bessel correction.
    PLUSS_CHECK(this->defined(), "var: undefined series");

    // Keep mean's dim so broadcasting works for subtraction.
    const Series m = this->mean(dim, /*keepdim=*/true);
    const Series diff = ops_internal::sub(*this, m);
    const Series sq = ops_internal::mul(diff, diff);

    Series v = ops_internal::mean(sq, dim, keepdim);

    if (unbiased) {
      const auto n = this->size(dim);
      if (n > 1) {
        const double corr = static_cast<double>(n) / static_cast<double>(n - 1);
        v = v.mul(corr);
      }
      // If n<=1, match torch-like behavior: return zeros (already implied by mean on empty/size1 paths).
    }
    return v;
  }

  Series Series::std(std::int64_t dim, bool unbiased, bool keepdim) const {
    // Standard deviation is sqrt(variance).
    const Series v = this->var(dim, unbiased, keepdim);
    return ops_internal::sqrt(v);
  }

  Series Series::norm(double p, std::int64_t dim, bool keepdim) const {
    // Basic p-norm along a dimension.
    // Supports p=1,2, +/-inf, and general finite p>0.
    PLUSS_CHECK(this->defined(), "norm: undefined series");

    if (std::isinf(p)) {
      const Series a = this->abs();
      if (p > 0) return a.amax(dim, keepdim);
      return a.amin(dim, keepdim);
    }

    PLUSS_CHECK(p > 0.0, "norm: p must be > 0 for now");

    const Series a = this->abs();

    if (p == 1.0) {
      return a.sum(dim, keepdim);
    }

    if (p == 2.0) {
      // sqrt(sum(x^2))
      const Series s = a.pow(2.0).sum(dim, keepdim);
      return s.pow(0.5);
    }

    // General p: (sum(|x|^p))^(1/p)
    const Series s = a.pow(p).sum(dim, keepdim);
    return s.pow(1.0 / p);
  }

  Series Series::pile(const std::vector<Series>& others, std::int64_t dim) const {
    // Method-first alias for stack: stack([self] + others, dim)
    std::vector<Series> all;
    all.reserve(1 + others.size());
    all.push_back(*this);
    all.insert(all.end(), others.begin(), others.end());
    return Series::stack(all, dim);
  }

  Series Series::unite(const std::vector<Series>& others, std::int64_t dim) const {
    // Method-first alias for cat: cat([self] + others, dim)
    std::vector<Series> all;
    all.reserve(1 + others.size());
    all.push_back(*this);
    all.insert(all.end(), others.begin(), others.end());
    return Series::cat(all, dim);
  }
}
