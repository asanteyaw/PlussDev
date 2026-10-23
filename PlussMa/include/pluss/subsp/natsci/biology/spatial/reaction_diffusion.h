#pragma once

// Reaction-diffusion systems on 1-3 dimensional cell-centred grids with periodic, Neumann (no-flux) and Dirichlet boundaries, batched over
// lanes. State layout [..., S, n_1 (, n_2 (, n_3))]: leading axes are independent lanes (parameter sweeps, initial conditions), then the
// species axis, then the spatial axes.
//
//   du_s/dt = D_s lap(u_s) + f_s(u),        f given as a callback acting on the whole state.
//
// Boundary handling follows the ghost-cell convention documented in py-pde (docs/methods/boundary_discretization, read from the GitHub
// checkout of zwicker-group/py-pde): a ghost value one cell outside the domain is set so that the boundary condition holds exactly midway
// between the ghost and the first real cell: Neumann  du/dn = B  -> ghost = u_edge + B h (outward normal derivative B; 0 = no flux),
// Dirichlet u = A on the boundary -> ghost = 2 A - u_edge, periodic -> wrap. The Laplacian is the second-order central difference.
// Time stepping: classical RK4 over a fixed number of steps (explicit; the stability bound dt <= h^2 / (2 d D) is reported by
// `max_stable_dt`) or an operator-split exponential step for the diffusion (see integrate_split).

#include "pluss/subsp/natsci/biology/dynamics.h"

#include <cmath>
#include <functional>
#include <vector>

namespace pluss::subsp::natsci::biology::spatial {

using dynamics::Coeff;

enum class Boundary { Periodic, Neumann, Dirichlet };

struct Axis final {
  std::int64_t cells{0};
  double length{1.0};
  Boundary lower = Boundary::Neumann, upper = Boundary::Neumann;
  double lower_value{0.0}, upper_value{0.0}; // Neumann: outward normal derivative, Dirichlet: boundary value
  [[nodiscard]] double spacing() const { return length / static_cast<double>(cells); }
};

struct Grid final {
  std::vector<Axis> axes; // 1 to 3 spatial axes
  [[nodiscard]] std::int64_t dim() const { return static_cast<std::int64_t>(axes.size()); }
};

inline Grid box(std::vector<std::int64_t> cells, std::vector<double> lengths, Boundary b = Boundary::Neumann) {
  dynamics::require(!cells.empty() && cells.size() <= 3 && cells.size() == lengths.size(), "spatial::box: one to three axes with matching lengths");
  Grid g;
  for (std::size_t i = 0; i < cells.size(); ++i) {
    dynamics::require(cells[i] >= 3 && lengths[i] > 0.0, "spatial::box: need at least three cells and a positive length per axis");
    g.axes.push_back({cells[i], lengths[i], b, b, 0.0, 0.0});
  }
  return g;
}

namespace detail {
// axis index counted from the end: spatial axis a of a grid of dimension d sits at u.dim() - d + a
inline std::int64_t axis_of(const Series &u, const Grid &g, std::int64_t a) { return u.dim() - g.dim() + a; }

// u padded by one ghost cell on both sides of spatial axis a
inline Series pad(const Series &u, const Grid &g, std::int64_t a) {
  const Axis &ax = g.axes[static_cast<std::size_t>(a)];
  const std::int64_t d = axis_of(u, g, a), n = ax.cells;
  const Series first = u.slice(d, 0, 1, 1), last = u.slice(d, n - 1, n, 1);
  const double h = ax.spacing();
  const auto ghost = [&](Boundary b, double value, const Series &edge, const Series &opposite) {
    switch (b) {
    case Boundary::Periodic: return opposite;
    case Boundary::Neumann: return edge.add(value * h);
    case Boundary::Dirichlet: return edge.mul(-1.0).add(2.0 * value);
    }
    return edge;
  };
  const Series lo = ghost(ax.lower, ax.lower_value, first, last);
  const Series hi = ghost(ax.upper, ax.upper_value, last, first);
  return lo.unite(std::vector<Series>{u, hi}, d);
}
} // namespace detail

// second-order central-difference Laplacian over the spatial axes
inline Series laplacian(const Series &u, const Grid &g) {
  Series sum = u.mul(0.0);
  for (std::int64_t a = 0; a < g.dim(); ++a) {
    const std::int64_t d = detail::axis_of(u, g, a), n = g.axes[static_cast<std::size_t>(a)].cells;
    const double h = g.axes[static_cast<std::size_t>(a)].spacing();
    const Series p = detail::pad(u, g, a);
    sum = sum.add(p.slice(d, 2, n + 2, 1).add(p.slice(d, 0, n, 1)).sub(u.mul(2.0)).div(h * h));
  }
  return sum;
}

using Reaction = std::function<Series(const Series &)>;

// D_s lap(u_s) + f(u) with per-species diffusion coefficients d [S] (scalar Coeff allowed for one-species systems)
inline Series rate(const Series &u, const Series &diffusion, const Grid &g, const Reaction &f) {
  std::vector<std::int64_t> shape(static_cast<std::size_t>(u.dim()), 1);
  shape[static_cast<std::size_t>(u.dim() - g.dim() - 1)] = diffusion.size(0);
  const Series d = diffusion.to(u.scalar_type()).to(u.platform()).reshape(shape);
  Series r = laplacian(u, g).mul(d);
  if (f)
    r = r.add(f(u));
  return r;
}

// explicit stability bound of forward-Euler-like steps for the diffusion part: dt <= 1 / (2 D_max sum_a 1/h_a^2); RK4 is stable up to
// about 2.78 / (that eigenvalue magnitude); this returns the Euler bound, use a safety factor <= 1.3 for RK4 (stability limit 2.78 / 2 = 1.39)
inline double max_stable_dt(const Grid &g, double d_max) {
  double s = 0.0;
  for (const auto &a : g.axes)
    s += 4.0 / (a.spacing() * a.spacing());
  return 2.0 / (d_max * s);
}

// classical RK4 for du/dt = rate(u); `steps` equal steps from t0 to t1
inline Series integrate(const Series &u0, const Series &diffusion, const Grid &g, const Reaction &f, double t0, double t1, std::int64_t steps) {
  const dynamics::RateFn fn = [&](const Series &u) { return rate(u, diffusion, g, f); };
  return dynamics::simulate(fn, u0, t0, t1, steps);
}

} // namespace pluss::subsp::natsci::biology::spatial
