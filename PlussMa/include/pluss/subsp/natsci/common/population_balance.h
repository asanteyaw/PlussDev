#pragma once

// Population balance equations (PBE) for a number density n(L, t) over a particle / cell / cluster size L, batched over lanes
// (every function accepts n of shape [..., B], B bins; leading axes are independent lanes, e.g. a parameter sweep or a batch of
// crystallizers).
//
//   dn/dt + d(G n)/dL = B0 delta(L - L0) + (birth - death by aggregation / breakage)           (growth + nucleation)
//
//  * Growth / nucleation / outflow: cell-centred finite volumes with a MUSCL reconstruction (van Leer slope limiter; van Leer, J. Comput.
//    Phys. 32 (1979) 101) of the face values, upwinded by the sign of G, an inflow flux B0 at the lower boundary (nucleation) and a
//    free outflow at the upper one; advanced by the strong-stability-preserving Runge-Kutta scheme of Shu & Osher (J. Comput. Phys.
//    77 (1988) 439) `ssprk3`. Second order on smooth profiles, monotone at fronts.
//  * Aggregation and breakage on a fixed pivot grid (Kumar & Ramkrishna, Chem. Eng. Sci. 51 (1996) 1311, 1333): every birth event is
//    shared between the two pivots that enclose the new size so that number and mass are preserved EXACTLY; the operators are dense
//    assignment tensors built once, so a right-hand side is a few matrix products over all lanes (no loops over bins or lanes).
//    With integer pivots 1, 2, 3, ... the assignment is exact (discrete Smoluchowski, Smoluchowski 1916).
//  * Moments mu_k = integral L^k n dL of the discretised density (exact for piecewise-constant cells).
//
// Conventions: bin edges e[0..B] (monotone), pivots / cell centres x[0..B-1]; sizes in any consistent unit.

#include "pluss/subsp/natsci/common/dynamics.h"

#include <cmath>
#include <functional>
#include <vector>

namespace pluss::subsp::natsci::common::population_balance {

using dynamics::Coeff;
using dynamics::RateFn;

// ---- grids ---------------------------------------------------------------------------------------------------
struct Grid final {
  std::vector<double> edges;   // B + 1
  std::vector<double> centers; // B
  [[nodiscard]] std::int64_t bins() const { return static_cast<std::int64_t>(centers.size()); }
  [[nodiscard]] double width(std::int64_t i) const { return edges[static_cast<std::size_t>(i) + 1] - edges[static_cast<std::size_t>(i)]; }
};

inline Grid uniform_grid(double lo, double hi, std::int64_t bins) {
  dynamics::require(bins >= 2 && hi > lo, "population_balance: need hi > lo and at least two bins");
  Grid g;
  for (std::int64_t i = 0; i <= bins; ++i)
    g.edges.push_back(lo + (hi - lo) * static_cast<double>(i) / static_cast<double>(bins));
  for (std::int64_t i = 0; i < bins; ++i)
    g.centers.push_back(0.5 * (g.edges[static_cast<std::size_t>(i)] + g.edges[static_cast<std::size_t>(i) + 1]));
  return g;
}

// geometric grid with pivots x_k = x0 ratio^k (edges at the geometric means), the usual choice for aggregation
inline Grid geometric_grid(double x0, double ratio, std::int64_t bins) {
  dynamics::require(x0 > 0.0 && ratio > 1.0 && bins >= 2, "population_balance: need x0 > 0, ratio > 1 and at least two bins");
  Grid g;
  for (std::int64_t k = 0; k < bins; ++k)
    g.centers.push_back(x0 * std::pow(ratio, static_cast<double>(k)));
  g.edges.push_back(g.centers.front() / std::sqrt(ratio));
  for (std::int64_t k = 0; k < bins; ++k)
    g.edges.push_back(g.centers[static_cast<std::size_t>(k)] * std::sqrt(ratio));
  return g;
}

// integer sizes 1, 2, ..., K as pivots (exact discrete Smoluchowski)
inline Grid integer_grid(std::int64_t bins) {
  Grid g;
  for (std::int64_t k = 0; k < bins; ++k) {
    g.centers.push_back(static_cast<double>(k) + 1.0);
    g.edges.push_back(static_cast<double>(k) + 0.5);
  }
  g.edges.push_back(static_cast<double>(bins) + 0.5);
  return g;
}

inline Series on(const std::vector<double> &v, const Series &like) {
  return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(like.scalar_type()).to(like.platform());
}

// ---- moments ---------------------------------------------------------------------------------------------------
// mu_k = sum_i n_i (e_{i+1}^{k+1} - e_i^{k+1}) / (k + 1): the exact k-th moment of the piecewise-constant density; [..., B] -> [...]
inline Series moment(const Series &n, const Grid &g, double k) {
  std::vector<double> w(static_cast<std::size_t>(g.bins()));
  for (std::int64_t i = 0; i < g.bins(); ++i)
    w[static_cast<std::size_t>(i)] = (std::pow(g.edges[static_cast<std::size_t>(i) + 1], k + 1.0) - std::pow(g.edges[static_cast<std::size_t>(i)], k + 1.0)) / (k + 1.0);
  return n.mul(on(w, n)).sum(-1, false);
}
// moments of the pivot-based (counts-per-bin) representation used by aggregation / breakage: sum_i x_i^k N_i
inline Series pivot_moment(const Series &counts, const Grid &g, double k) {
  std::vector<double> w(static_cast<std::size_t>(g.bins()));
  for (std::int64_t i = 0; i < g.bins(); ++i)
    w[static_cast<std::size_t>(i)] = std::pow(g.centers[static_cast<std::size_t>(i)], k);
  return counts.mul(on(w, counts)).sum(-1, false);
}

// ---- time stepping ---------------------------------------------------------------------------------------------
// Shu-Osher SSP Runge-Kutta of order 3 for autonomous dn/dt = f(n); `steps` equal steps of size dt = (t1 - t0) / steps
inline Series ssprk3(const RateFn &f, const Series &n0, double t0, double t1, std::int64_t steps) {
  dynamics::require(steps >= 1 && t1 >= t0, "ssprk3: need steps >= 1 and t1 >= t0");
  const double dt = (t1 - t0) / static_cast<double>(steps);
  Series n = n0;
  for (std::int64_t s = 0; s < steps; ++s) {
    const Series n1 = n.add(f(n).mul(dt));
    const Series n2 = n.mul(0.75).add(n1.add(f(n1).mul(dt)).mul(0.25));
    n = n.mul(1.0 / 3.0).add(n2.add(f(n2).mul(dt)).mul(2.0 / 3.0));
  }
  return n;
}

// ---- growth, nucleation, outflow -------------------------------------------------------------------------------
// dn/dt contribution of  d(G n)/dL = ... : growth rate at the B + 1 bin edges (G >= 0 and G < 0 both supported),
// `nucleation_flux` = B0 = G(0) n(0) enters at the lowest edge (scalar or [...] per lane; 0 = none), the highest edge is a free outflow.
inline Series growth_rhs(const Series &n, const Grid &g, const Series &growth_at_edges, const Coeff &nucleation_flux = 0.0) {
  const std::int64_t b = g.bins();
  dynamics::require(n.size(-1) == b, "growth_rhs: n must have one entry per bin");
  const Series first = n.slice(-1, 0, 1, 1), last = n.slice(-1, b - 1, b, 1);
  const Series g0_edge = growth_at_edges.slice(-1, 0, 1, 1);
  // lower ghost cell: linear extrapolation through the inflow state B0 / G(0) at L = 0 (keeps the first cell second order); free outflow
  // above (zero-gradient ghost). With G(0) <= 0 there is no inflow and the ghost is zero-gradient.
  const Series inflow_state = nucleation_flux.on(n.select(-1, 0)).unsqueeze(-1).div(g0_edge.gt(0.0).where(g0_edge, g0_edge.mul(0.0).add(1.0)));
  const Series ghost_lo = g0_edge.gt(0.0).where(inflow_state.mul(2.0).sub(first), first);
  const Series ext = ghost_lo.unite(std::vector<Series>{n, last}, -1); // [..., B + 2]
  const Series left = ext.slice(-1, 0, b, 1), mid = ext.slice(-1, 1, b + 1, 1), right = ext.slice(-1, 2, b + 2, 1);
  const Series da = mid.sub(left), db = right.sub(mid);
  const Series prod = da.mul(db);
  // van Leer slope: 2 a b / (a + b) when a b > 0, else 0 (no division by zero where the mask is false)
  const Series denom = da.add(db);
  const Series safe = prod.gt(0.0).where(denom, denom.mul(0.0).add(1.0));
  const Series slope = prod.gt(0.0).where(prod.mul(2.0).div(safe), prod.mul(0.0)); // [..., B], slope of cell i
  // face i + 1/2 (i = 0 .. B - 2) values from the left cell i and the right cell i + 1
  const Series face_left = mid.add(slope.mul(0.5)).slice(-1, 0, b - 1, 1);
  const Series face_right = mid.slice(-1, 1, b, 1).sub(slope.slice(-1, 1, b, 1).mul(0.5));
  const Series gi = growth_at_edges.slice(-1, 1, b, 1); // interior faces
  const Series interior = gi.ge(0.0).where(gi.mul(face_left), gi.mul(face_right)); // [..., B - 1]
  const Series g0 = growth_at_edges.slice(-1, 0, 1, 1), gb = growth_at_edges.slice(-1, b, b + 1, 1);
  const Series flux0 = nucleation_flux.on(n.select(-1, 0)).unsqueeze(-1).add(g0.mul(0.0)); // inflow B0 at L = 0
  const Series fluxb = gb.mul(last);                                                      // outflow
  const Series flux = flux0.unite(std::vector<Series>{interior, fluxb}, -1);                // [..., B + 1]
  std::vector<double> inv_w(static_cast<std::size_t>(b));
  for (std::int64_t i = 0; i < b; ++i)
    inv_w[static_cast<std::size_t>(i)] = 1.0 / g.width(i);
  const Series dflux = flux.slice(-1, 1, b + 1, 1).sub(flux.slice(-1, 0, b, 1));
  return dflux.mul(on(inv_w, n)).mul(-1.0);
}

// ---- aggregation / breakage on a fixed pivot grid ------------------------------------------------------------
// Working variable: N_i = number of particles in pivot bin i (number per volume), not a density.
struct AggregationOperator final {
  Series gain_tensor = pluss::series::undefined(); // [B * B, B]: share of a pair (i, j) event assigned to bin k, row index i * B + j
  Series pair_weight = pluss::series::undefined(); // [B * B]: 1 for i < j and 1/2 for i = j ... see aggregation_rhs
  std::int64_t bins{0};
};

// birth of size v from a pair (x_i, x_j): shared between the enclosing pivots so number (a + b = 1) and mass are preserved
inline AggregationOperator build_aggregation_operator(const Grid &g, const Series &like) {
  const std::int64_t b = g.bins();
  std::vector<double> t(static_cast<std::size_t>(b * b * b), 0.0);
  for (std::int64_t i = 0; i < b; ++i)
    for (std::int64_t j = 0; j < b; ++j) {
      const double v = g.centers[static_cast<std::size_t>(i)] + g.centers[static_cast<std::size_t>(j)];
      const std::size_t row = static_cast<std::size_t>(i * b + j);
      if (v >= g.centers.back()) { // beyond the grid: the mass is kept in the last pivot (number-conserving truncation)
        t[row * static_cast<std::size_t>(b) + static_cast<std::size_t>(b - 1)] = 1.0;
        continue;
      }
      std::int64_t k = 0;
      while (k + 1 < b && g.centers[static_cast<std::size_t>(k) + 1] <= v + 1e-12 * v)
        ++k;
      const double xk = g.centers[static_cast<std::size_t>(k)];
      if (std::abs(v - xk) <= 1e-12 * v || k + 1 >= b) {
        t[row * static_cast<std::size_t>(b) + static_cast<std::size_t>(k)] = 1.0;
      } else {
        const double xk1 = g.centers[static_cast<std::size_t>(k) + 1];
        t[row * static_cast<std::size_t>(b) + static_cast<std::size_t>(k)] = (xk1 - v) / (xk1 - xk);
        t[row * static_cast<std::size_t>(b) + static_cast<std::size_t>(k) + 1] = (v - xk) / (xk1 - xk);
      }
    }
  AggregationOperator op;
  op.bins = b;
  op.gain_tensor = pluss::make_series(t, {b * b, b}).to(like.scalar_type()).to(like.platform());
  return op;
}

// dN/dt = 1/2 sum_{i,j} eta_{ijk} K_ij N_i N_j - N_k sum_j K_kj N_j ; kernel [B, B] (symmetric) or [..., B, B] per lane
inline Series aggregation_rhs(const Series &counts, const AggregationOperator &op, const Series &kernel) {
  const std::int64_t b = op.bins;
  const Series ni = counts.unsqueeze(-1), nj = counts.unsqueeze(-2);
  const Series pair = ni.mul(nj).mul(kernel); // [..., B, B]: K_ij N_i N_j
  std::vector<std::int64_t> shape;
  for (std::int64_t d = 0; d + 2 < pair.dim(); ++d)
    shape.push_back(pair.size(d));
  shape.push_back(b * b);
  const Series flat = pair.contiguous().reshape(shape); // [..., B, B] -> [..., B * B], row index i * B + j
  const Series gain = flat.matmul(op.gain_tensor).mul(0.5);
  const Series loss = counts.mul(kernel.mul(nj).sum(-1, false));
  return gain.sub(loss);
}

struct BreakageOperator final {
  Series daughters = pluss::series::undefined(); // [B, B]: row j (parent), column k: expected number of fragments landing on pivot k
  Series mother_loss = pluss::series::undefined();
  std::int64_t bins{0};
};

// daughter distribution beta(x | y) (number of daughters of size x per breakage of size y; integrates to nu(y) and conserves mass:
// integral x beta dx = y), integrated over every bin by Gauss-Legendre quadrature and shared between the enclosing pivots so that
// number and mass of the fragments are preserved. beta(x, y) is a host callback.
inline BreakageOperator build_breakage_operator(const Grid &g, const std::function<double(double, double)> &beta, const Series &like, int quad_points = 8) {
  const std::int64_t b = g.bins();
  // Gauss-Legendre nodes / weights on [-1, 1] by Newton iteration on P_n
  std::vector<double> xi(static_cast<std::size_t>(quad_points)), wi(static_cast<std::size_t>(quad_points));
  for (int i = 0; i < quad_points; ++i) {
    double z = std::cos(M_PI * (i + 0.75) / (quad_points + 0.5)), pp = 0.0;
    for (int it = 0; it < 100; ++it) {
      double p1 = 1.0, p2 = 0.0;
      for (int j = 0; j < quad_points; ++j) {
        const double p3 = p2;
        p2 = p1;
        p1 = ((2.0 * j + 1.0) * z * p2 - j * p3) / (j + 1.0);
      }
      pp = quad_points * (z * p1 - p2) / (z * z - 1.0);
      const double dz = p1 / pp;
      z -= dz;
      if (std::abs(dz) < 1e-15)
        break;
    }
    xi[static_cast<std::size_t>(i)] = z;
    wi[static_cast<std::size_t>(i)] = 2.0 / ((1.0 - z * z) * pp * pp);
  }
  std::vector<double> d(static_cast<std::size_t>(b * b), 0.0);
  for (std::int64_t j = 0; j < b; ++j) {
    const double y = g.centers[static_cast<std::size_t>(j)];
    for (std::int64_t k = -1; k <= j; ++k) { // daughters of a parent of size y are not larger than y; k = -1 is the range below the grid
      const double lo = k < 0 ? 0.0 : g.edges[static_cast<std::size_t>(k)];
      const double hi = std::min(k < 0 ? g.edges.front() : g.edges[static_cast<std::size_t>(k) + 1], y);
      if (hi <= lo)
        continue;
      double number = 0.0, mass = 0.0;
      for (int q = 0; q < quad_points; ++q) {
        const double x = 0.5 * (hi - lo) * xi[static_cast<std::size_t>(q)] + 0.5 * (hi + lo);
        const double w = 0.5 * (hi - lo) * wi[static_cast<std::size_t>(q)];
        const double bv = beta(x, y);
        number += w * bv;
        mass += w * bv * x;
      }
      if (number <= 0.0)
        continue;
      // fragments of mean size mass/number inside bin k: split between the enclosing pivots (preserves number and mass)
      const double xbar = mass / number;
      std::int64_t lower = std::max<std::int64_t>(k, 0);
      while (lower > 0 && g.centers[static_cast<std::size_t>(lower)] > xbar)
        --lower;
      while (lower + 1 < b && g.centers[static_cast<std::size_t>(lower) + 1] <= xbar)
        ++lower;
      if (lower + 1 >= b || xbar <= g.centers[static_cast<std::size_t>(lower)]) {
        // below the lowest pivot (or at / above the highest): keep the mass by adjusting the number, so mass is preserved
        d[static_cast<std::size_t>(j * b + lower)] += mass / g.centers[static_cast<std::size_t>(lower)];
      } else {
        const double x0 = g.centers[static_cast<std::size_t>(lower)], x1 = g.centers[static_cast<std::size_t>(lower) + 1];
        const double w1 = (xbar - x0) / (x1 - x0), w0 = 1.0 - w1;
        d[static_cast<std::size_t>(j * b + lower)] += number * w0;
        d[static_cast<std::size_t>(j * b + lower + 1)] += number * w1;
      }
    }
  }
  BreakageOperator op;
  op.bins = b;
  op.daughters = pluss::make_series(d, {b, b}).to(like.scalar_type()).to(like.platform());
  return op;
}

// dN/dt = sum_j S_j N_j D_jk - S_k N_k ; selection rate S [B] or [..., B]
inline Series breakage_rhs(const Series &counts, const BreakageOperator &op, const Series &selection) {
  const Series broken = counts.mul(selection);
  return broken.matmul(op.daughters).sub(broken);
}

} // namespace pluss::subsp::natsci::common::population_balance
