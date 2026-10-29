#pragma once

#include "pluss/codem/codem.h"
#include "pluss/subsp/math/linalg.h"

#include <algorithm>
#include <cstdint>

namespace pluss::subsp::stats::regression::detail {

// The numerical rank of a design matrix and the cutoff that defines it. A singular value counts when it exceeds
// rcond * max(n, k) * max(largest, 1); `pinv_rcond` is the same cutoff written as the relative cutoff `pinverse` expects, so the
// pseudoinverse that produces the coefficients discards exactly the directions that `rank` leaves out.
struct DesignRank {
  std::int64_t rank = 0;
  double largest = 0.0;
  double smallest = 0.0;
  double pinv_rcond = 0.0;
};

inline DesignRank design_rank(const pluss::Series& design, double rcond) {
  const std::int64_t n = design.sizes()[0];
  const std::int64_t k = design.sizes()[1];
  const auto decomposition = pluss::subsp::math::linalg::svd(design, false);
  const pluss::Series singular =
      decomposition.S.reshape({decomposition.S.numel()}).to(design.platform(), pluss::ScalarType::Float64);
  DesignRank out;
  out.largest = singular.max().item<double>();
  out.smallest = singular.min().item<double>();
  const double cutoff = rcond * static_cast<double>(std::max<std::int64_t>(n, k)) * std::max(out.largest, 1.0);
  out.rank = static_cast<std::int64_t>(singular.gt(cutoff).to(pluss::ScalarType::Float64).sum().item<double>() + 0.5);
  out.pinv_rcond = out.largest > 0.0 ? cutoff / out.largest : rcond;
  return out;
}

} // namespace pluss::subsp::stats::regression::detail
