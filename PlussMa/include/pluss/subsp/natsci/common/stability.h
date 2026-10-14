#pragma once

// Linear stability of equilibria without an eigensolver, batched over lanes:
//   * characteristic_polynomial: Faddeev-LeVerrier recursion
//       M_0 = 0, c_0 = 1;  M_k = A M_{k-1} + c_{k-1} I,  c_k = -tr(A M_k)/k,
//     so det(lambda I - A) = sum_k c_k lambda^{n-k}   (c_0 = 1).
//     n steps of batched matmuls; accurate for the small Jacobians (n <~ 10)
//     of reaction / population / circuit models.
//   * routh: the Routh array of a monic real polynomial; the number of sign
//     changes in its first column equals the number of roots in the open right
//     half plane (Routh 1877; Hurwitz 1895). A zero pivot is perturbed by the
//     epsilon method so the count stays defined; a whole zero row (roots on the
//     imaginary axis, e.g. the neutral centre of Lotka-Volterra) is flagged
//     `marginal`.
//   (Hopf / saddle-node thresholds in a parameter are located with the shared
//   batched solvers of pluss/subsp/math/roots.h, e.g. roots::bisect on the
//   unstable-mode count.)

#include "pluss/subsp/math/roots.h"
#include "pluss/subsp/natsci/common/dynamics.h"

namespace pluss::subsp::natsci::common::stability {

// [..., n, n] -> [..., n + 1], coefficients c_0 .. c_n of det(lambda I - A).
inline Series characteristic_polynomial(const Series &a) {
  dynamics::require_square(a, "stability: Jacobian");
  const std::int64_t n = a.size(-1);
  const Config cfg = Config{}.with_dtype(a.scalar_type()).with_platform(a.platform());
  const Series eye = pluss::series::eye(n, cfg);
  Series m = a.mul(0.0);
  Series c = a.select(-1, 0).select(-1, 0).mul(0.0).add(1.0); // c_0 = 1, shape [...]
  std::vector<Series> coeffs{c};
  for (std::int64_t k = 1; k <= n; ++k) {
    m = a.matmul(m).add(eye.mul(c.unsqueeze(-1).unsqueeze(-1)));
    const Series trace = a.matmul(m).mul(eye).sum(-1, false).sum(-1, false);
    c = trace.div(-static_cast<double>(k));
    coeffs.push_back(c);
  }
  Series out = coeffs.front().unsqueeze(-1);
  std::vector<Series> tail;
  for (std::size_t i = 1; i < coeffs.size(); ++i)
    tail.push_back(coeffs[i].unsqueeze(-1));
  return out.unite(tail, -1);
}

struct Routh final {
  Series unstable = pluss::series::undefined(); // [...] number of roots with Re > 0
  Series first_column = pluss::series::undefined(); // [..., n + 1]
  Series marginal = pluss::series::undefined();  // [...] 1 where a pivot vanished (imaginary-axis roots)
};

// coeffs [..., n + 1] with coeffs[0] = 1 (monic) or any non-zero leading term.
inline Routh routh(const Series &coeffs, double epsilon = 1e-9) {
  const std::int64_t n = coeffs.size(-1) - 1;
  dynamics::require(n >= 1, "stability: polynomial needs degree >= 1");
  const std::int64_t width = (n + 2) / 2;
  const Series zero = coeffs.select(-1, 0).mul(0.0);
  const Series zero_col = zero.unsqueeze(-1);
  const Series padded_coeffs = coeffs.unite(std::vector<Series>{zero_col}, -1); // trailing zero at index n + 1
  const auto initial_row = [&](std::int64_t start) {
    std::vector<double> index;
    for (std::int64_t j = 0; j < width; ++j)
      index.push_back(static_cast<double>(std::min<std::int64_t>(start + 2 * j, n + 1)));
    const Series idx = pluss::make_series(index, {width}).to(pluss::ScalarType::Int64).to(coeffs.platform());
    return padded_coeffs.index_select(-1, idx);                                   // [..., width]
  };
  const Series scale = coeffs.abs().amax(-1, false).clamp(1e-300, 1e300);
  Series r0 = initial_row(0), r1 = initial_row(1);
  std::vector<Series> first{r0.select(-1, 0), r1.select(-1, 0)};
  Series marginal = zero;
  for (std::int64_t row = 2; row <= n; ++row) {
    Series pivot = r1.select(-1, 0);
    const Series tiny = pivot.abs().lt(epsilon * scale);
    // Imaginary-axis roots show up as an entire row of zeros; a lone zero pivot with a
    // non-zero rest of the row (e.g. a zero polynomial coefficient) is handled by the
    // epsilon method and is NOT marginal.
    const Series row_zero = r1.abs().amax(-1, false).lt(epsilon * scale);
    marginal = marginal.add(tiny.logical_and(row_zero).to(coeffs.scalar_type())).clamp(0.0, 1.0);
    // epsilon method: a vanishing pivot becomes a small positive number
    pivot = tiny.where(scale.mul(epsilon), pivot);
    r1 = pivot.unsqueeze(-1).unite(std::vector<Series>{r1.slice(-1, 1, width, 1)}, -1);
    // b_j = (r1[0] r0[j+1] - r0[0] r1[j+1]) / r1[0], the whole row at once
    const Series p = pivot.unsqueeze(-1);
    const Series body = p.mul(r0.slice(-1, 1, width, 1)).sub(r0.slice(-1, 0, 1, 1).mul(r1.slice(-1, 1, width, 1))).div(p);
    const Series next = body.unite(std::vector<Series>{zero_col}, -1);
    r0 = r1;
    r1 = next;
    first.push_back(r1.select(-1, 0));
  }
  Series col = first.front().unsqueeze(-1);
  std::vector<Series> tail;
  for (std::size_t i = 1; i < first.size(); ++i)
    tail.push_back(first[i].unsqueeze(-1));
  col = col.unite(tail, -1);
  // sign changes between consecutive first-column entries
  const Series prod = col.slice(-1, 0, n, 1).mul(col.slice(-1, 1, n + 1, 1));
  const Series changes = prod.lt(0.0).to(coeffs.scalar_type()).sum(-1, false);
  return {changes, col, marginal};
}

// Number of eigenvalues of A with positive real part (and a marginal flag).
inline Routh unstable_modes(const Series &a, double epsilon = 1e-9) {
  return routh(characteristic_polynomial(a), epsilon);
}

// Local stability of a rate map at states x [..., n]: scaled central-difference Jacobian
// (all lanes and columns in one batched call) followed by the Routh-Hurwitz count.
inline Routh local_stability(const dynamics::RateFn &rate, const Series &x, double relative_step = 1e-6) {
  const Series step = x.abs().clamp(1e-3, 1e300).mul(relative_step);
  return unstable_modes(dynamics::jacobian_scaled(rate, x, step));
}

} // namespace pluss::subsp::natsci::common::stability
