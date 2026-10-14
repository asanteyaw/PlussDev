#pragma once

// Age/stage-structured (matrix) populations. Matrices are [..., k, k];
// leading axes are independent lanes. Eigen-quantities use shifted power
// iteration (M + I) on M and M^T: it converges for any non-negative matrix
// with a Perron root, including imprimitive Leslie matrices, and needs no
// general eigensolver. Sensitivities follow Caswell (2001) / popbio:
//   s_ij = w_i v_j / <w, v>,  e_ij = a_ij s_ij / lambda
// (w left, v right eigenvector). Layout per Wikipedia "Matrix population
// models": fertilities on the first row, survivals on the subdiagonal.

#include "pluss/subsp/natsci/biology/dynamics.h"

namespace pluss::subsp::natsci::biology::population::structured {

// fertility [..., k], survival [..., k-1].
inline Series leslie(const Series &fertility, const Series &survival) {
  const std::int64_t k = fertility.size(-1);
  dynamics::require(survival.size(-1) == k - 1,
                    "leslie: survival must have k-1 entries");
  dynamics::require_nonnegative(fertility, "leslie: fertility");
  dynamics::require_nonnegative(survival, "leslie: survival");
  dynamics::require(survival.le(1.0).all().item<bool>(),
                    "leslie: survival must not exceed 1");
  const Config cfg = Config{}.with_dtype(fertility.scalar_type())
                         .with_platform(fertility.platform());
  const Series row0 = fertility.unsqueeze(-2);
  // rows 1..k-1: survival_i * e_{i-1}
  const Series shifted =
      pluss::series::eye(k - 1, cfg)
          .unite(std::vector<Series>{pluss::series::zeros({k - 1, 1}, cfg)}, 1);
  const Series lower = survival.unsqueeze(-1).mul(shifted);
  return row0.unite(std::vector<Series>{lower}, -2);
}

// n_{t+steps} = M^steps n_t (sequential in time, batched over lanes).
inline Series project(const Series &matrix, const Series &n,
                      std::int64_t steps) {
  dynamics::require_square(matrix, "project: matrix");
  dynamics::require(steps >= 0, "project: steps must be non-negative");
  Series y = n;
  for (std::int64_t i = 0; i < steps; ++i)
    y = dynamics::matvec(matrix, y);
  return y;
}

struct Eigen final {
  Series lambda = pluss::series::undefined(); // [...]
  Series right = pluss::series::undefined();  // [..., k], sum = 1
  Series left = pluss::series::undefined();   // [..., k], normalised w.v = 1
  std::int64_t iterations{0};
  bool converged{false};
};

namespace detail {
inline Series power(const Series &m, std::int64_t max_iter, double tol,
                    std::int64_t &used, bool &converged) {
  const std::int64_t k = m.size(-1);
  const Config cfg = Config{}.with_dtype(m.scalar_type()).with_platform(m.platform());
  const Series shifted = m.add(pluss::series::eye(k, cfg));
  Series v = pluss::series::ones({k}, cfg).div(static_cast<double>(k));
  v = v.expand([&] {
    std::vector<std::int64_t> s;
    for (std::int64_t i = 0; i + 1 < m.dim(); ++i)
      s.push_back(m.size(i));
    return s;
  }());
  converged = false;
  for (std::int64_t i = 1; i <= max_iter; ++i) {
    Series next = dynamics::matvec(shifted, v);
    next = next.div(next.sum(-1, true));
    used = i;
    if (i % 16 == 0 || i == max_iter) {
      const double d = next.sub(v).abs().amax(-1, false).max().template item<double>();
      v = next;
      if (d < tol) {
        converged = true;
        break;
      }
    } else {
      v = next;
    }
  }
  return v;
}
} // namespace detail

inline Eigen dominant(const Series &matrix, std::int64_t max_iterations = 20000,
                      double tolerance = 1e-14) {
  dynamics::require_square(matrix, "dominant: matrix");
  dynamics::require_nonnegative(matrix, "dominant: matrix");
  Eigen out;
  bool c1 = false, c2 = false;
  std::int64_t i1 = 0, i2 = 0;
  out.right = detail::power(matrix, max_iterations, tolerance, i1, c1);
  Series w = detail::power(matrix.transpose(-1, -2), max_iterations, tolerance,
                           i2, c2);
  const Series mv = dynamics::matvec(matrix, out.right);
  out.lambda = mv.sum(-1, false).div(out.right.sum(-1, false));
  out.left = w.div(w.mul(out.right).sum(-1, true));
  out.iterations = std::max(i1, i2);
  out.converged = c1 && c2;
  return out;
}

inline Series sensitivity(const Eigen &e) {
  return e.left.unsqueeze(-1).mul(e.right.unsqueeze(-2));
}

inline Series elasticity(const Series &matrix, const Eigen &e) {
  return matrix.mul(sensitivity(e)).div(e.lambda.unsqueeze(-1).unsqueeze(-1));
}

// Net reproductive rate of a Leslie matrix: sum_x F_x l_x with
// l_x = prod_{j<x} S_j; generation time T = ln R0 / ln lambda.
inline Series net_reproductive_rate(const Series &fertility,
                                    const Series &survival) {
  const Config cfg = Config{}.with_dtype(fertility.scalar_type())
                         .with_platform(fertility.platform());
  std::vector<std::int64_t> head;
  for (std::int64_t i = 0; i + 1 < fertility.dim(); ++i)
    head.push_back(fertility.size(i));
  head.push_back(1);
  const Series lx = pluss::series::ones(head, cfg).unite(
      std::vector<Series>{survival.cumprod(-1)}, -1);
  return fertility.mul(lx).sum(-1, false);
}

inline Series generation_time(const Series &r0, const Series &lambda) {
  return r0.log().div(lambda.log());
}

} // namespace pluss::subsp::natsci::biology::population::structured
