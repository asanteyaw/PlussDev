#pragma once

// Absorbing finite Markov chains (states 0..S with 0 and S absorbing), the
// exact reference for fixation problems. For the transient block Q (states
// 1..S-1) and the one-step probability r of entering state S:
//   fixation  h = (I - Q)^-1 r,     expected absorption time t = (I - Q)^-1 1.
// (Standard fundamental-matrix results for absorbing chains.) Rows of P may
// be batched: P is [..., S+1, S+1].

#include "pluss/subsp/natsci/biology/dynamics.h"

namespace pluss::subsp::natsci::biology::genetics {

struct Absorption final {
  Series fixation = pluss::series::undefined(); // [..., S+1], h_0 = 0, h_S = 1
  Series time = pluss::series::undefined();     // [..., S+1], 0 at the ends
};

inline Absorption absorption(const Series &p) {
  dynamics::require_square(p, "absorption: transition matrix");
  dynamics::require_nonnegative(p, "absorption: transition matrix");
  const std::int64_t n = p.size(-1);
  dynamics::require(n >= 3, "absorption: need at least one transient state");
  dynamics::require(
      p.sum(-1, false).sub(1.0).abs().max().item<double>() < 1e-9,
      "absorption: rows must sum to one");
  const Config cfg =
      Config{}.with_dtype(p.scalar_type()).with_platform(p.platform());
  const Series q = p.slice(-2, 1, n - 1, 1).slice(-1, 1, n - 1, 1);
  const Series r = p.slice(-2, 1, n - 1, 1).select(-1, n - 1);
  const Series a = pluss::series::eye(n - 2, cfg).sub(q);
  const Series inverse = pluss::subsp::linearalg::inverse(a);
  const Series h = inverse.matmul(r.unsqueeze(-1)).squeeze(-1);
  const Series t = inverse.sum(-1, false);
  const Series zero = h.slice(-1, 0, 1, 1).mul(0.0);
  const Series one = zero.add(1.0);
  return {zero.unite(std::vector<Series>{h, one}, -1),
          zero.unite(std::vector<Series>{t, zero}, -1)};
}

} // namespace pluss::subsp::natsci::biology::genetics
