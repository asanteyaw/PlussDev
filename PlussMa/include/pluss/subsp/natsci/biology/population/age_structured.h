#pragma once

// Continuous age structure (McKendrick-von Foerster, Gurtin-MacCamy), batched over lanes.
//
//   dn/dt + dn/da = -mu(a, N) n,     n(0, t) = B(t) = integral beta(a, N) n(a, t) da,     N(t) = integral n da
//
// Method of characteristics on the diagonal grid  dt = da = h  (ages a_j = j h, j = 0..A-1, the last node is a plus group that accumulates the
// survivors): the transport is an exact shift, mortality is integrated exactly along the characteristic (midpoint rule on mu), the birth integral and N
// are trapezoid sums (Series::antidiff().trapezoid) and the newborn node is solved implicitly, so the scheme is second-order, has no numerical diffusion and no
// CFL limit; density dependence is resolved by a predictor-corrector sweep. The population-balance engine in common::population_balance (MUSCL transport) is
// the right tool when the "age" velocity is not constant; for unit-speed ageing the shift is exact and cheaper.
// Verified against: exact transport with constant mortality, the continuous Euler-Lotka rate r = beta0 - c - mu (second-order grid convergence) and the
// closed-form Gurtin-MacCamy equilibrium. McKendrick (1926), von Foerster (1959), Gurtin & MacCamy (1974) were not obtained.

#include "pluss/subsp/natsci/biology/dynamics.h"
#include "pluss/subsp/natsci/biology/population/demography.h"

#include <cmath>
#include <functional>
#include <numbers>

namespace pluss::subsp::natsci::biology::population::age_structured {

namespace demography = ::pluss::subsp::natsci::biology::population::demography;

// rate callbacks: (ages [A], total population N [...]) -> rate [..., A]
using Rate = std::function<Series(const Series &ages, const Series &total)>;

struct Model final {
  Rate mortality;                  // mu(a, N)
  Rate fecundity;                  // beta(a, N)
  bool density_dependent{false};   // true: mortality / fecundity read N (predictor-corrector, two evaluations per step)
};
struct Grid final {
  double step{0.1};
  std::int64_t nodes{200};
  [[nodiscard]] Series ages(const Config &cfg = Config{}.with_dtype(pluss::ScalarType::Float64)) const { return pluss::series::arange(0, nodes, 1, cfg.dtype).mul(step); }
  [[nodiscard]] double maximum_age() const { return step * static_cast<double>(nodes - 1); }
};

[[nodiscard]] inline Series total(const Series &n, const Grid &grid) { return n.antidiff().trapezoid(grid.step, -1); }
[[nodiscard]] inline Series births(const Series &n, const Series &beta, const Grid &grid) { return n.mul(beta).antidiff().trapezoid(grid.step, -1); }

namespace detail {
// n shifted one node up the age axis (node 0 zero), plus-group survivors kept at the last node
inline Series advance(const Series &n, const Series &survive) {
  const std::int64_t a = n.size(-1);
  const Series moved = n.mul(survive);                                   // survivors of one step at each node
  const Series shifted = moved.slice(-1, 0, a - 1, 1);                   // nodes 0..A-2 move to 1..A-1
  const Series zero = n.slice(-1, 0, 1, 1).mul(0.0);
  const Series next = zero.unite(std::vector<Series>{shifted}, n.dim() - 1);
  const Series stay = moved.slice(-1, 0, a - 1, 1).mul(0.0).unite(std::vector<Series>{moved.slice(-1, a - 1, a, 1)}, n.dim() - 1); // plus group survivors stay
  return next.add(stay);
}
} // namespace detail

// one step of length h
[[nodiscard]] inline Series step(const Series &n, const Model &model, const Grid &grid) {
  const Series ages = grid.ages(Config{}.with_dtype(n.scalar_type()).with_platform(n.platform()));
  const double h = grid.step;
  const Series half = ages.add(0.5 * h);
  const auto advance_with = [&](const Series &total_for_rates) {
    const Series survive = model.mortality(half, total_for_rates).mul(-h).exp();
    return detail::advance(n, survive);
  };
  Series next = advance_with(total(n, grid));
  const auto finish = [&](const Series &aged, const Series &n_for_rates) { // fill the newborn node implicitly
    const Series beta = model.fecundity(ages, total(n_for_rates, grid));
    const Series rest = aged.mul(beta).antidiff().trapezoid(h, -1);                    // node 0 is zero in `aged`
    const Series b0 = beta.select(-1, 0);
    const Series newborn = rest.div(b0.mul(-0.5 * h).add(1.0));
    return aged.slice(-1, 0, 1, 1).mul(0.0).add(newborn.unsqueeze(-1)).unite(std::vector<Series>{aged.slice(-1, 1, aged.size(-1), 1)}, n.dim() - 1);
  };
  next = finish(next, n);
  if (model.density_dependent) {                                            // corrector: rates at the mean of the old and the predicted population
    const Series mid = n.add(next).mul(0.5);
    next = finish(advance_with(total(mid, grid)), mid);
  }
  return next;
}

[[nodiscard]] inline Series simulate(const Series &n0, const Model &model, const Grid &grid, std::int64_t steps) {
  dynamics::require(steps >= 0 && n0.size(-1) == grid.nodes, "age_structured::simulate: steps >= 0 and one density value per age node");
  Series n = n0;
  for (std::int64_t i = 0; i < steps; ++i)
    n = step(n, model, grid);
  return n;
}
// trajectory [..., steps + 1, A]
[[nodiscard]] inline Series trajectory(const Series &n0, const Model &model, const Grid &grid, std::int64_t steps) {
  std::vector<Series> path{n0};
  for (std::int64_t i = 0; i < steps; ++i)
    path.push_back(step(path.back(), model, grid));
  return path.front().pile(std::vector<Series>(path.begin() + 1, path.end()), n0.dim() - 1);
}

// ---- linear (density-independent) theory -----------------------------------------------------------------------------------------------
// survivorship l(a) = exp(-int_0^a mu) by the trapezoid rule on the grid: [..., A]
[[nodiscard]] inline Series survivorship(const Series &mortality_at_nodes, const Grid &grid) {
  const Series cumulative = mortality_at_nodes.antidiff().cumulative_trapezoid(grid.step, -1);   // [..., A - 1]
  return cumulative.slice(-1, 0, 1, 1).mul(0.0).unite(std::vector<Series>{cumulative}, mortality_at_nodes.dim() - 1).neg().exp();
}
// intrinsic rate r and R0 from mu(a), beta(a) at the nodes: trapezoid Euler-Lotka equation  int beta l e^{-r a} da = 1  (reuses demography::life_table)
[[nodiscard]] inline demography::LifeTable euler_lotka(const Series &mortality_at_nodes, const Series &fecundity_at_nodes, const Grid &grid) {
  const Series l = survivorship(mortality_at_nodes, grid);
  const Series ages = grid.ages(Config{}.with_dtype(l.scalar_type()).with_platform(l.platform()));
  std::vector<double> w(static_cast<std::size_t>(grid.nodes), grid.step);
  w.front() = w.back() = 0.5 * grid.step;
  return demography::life_table(l, fecundity_at_nodes, ages, 80, pluss::make_series(w, {grid.nodes}).to(l.scalar_type()).to(l.platform()));
}

} // namespace pluss::subsp::natsci::biology::population::age_structured
