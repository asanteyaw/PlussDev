#pragma once

// Constraint-based metabolic analysis on a stoichiometric network:   S v = 0,   l <= v <= u.
//   flux_balance        max c^T v                                     (Orth, Thiele & Palsson 2010 "What is flux balance analysis?")
//   flux_variability    [min v_j, max v_j] for every reaction subject to c^T v >= f z*   (Mahadevan & Schilling 2003)
//   knockouts / sweeps  one lane per perturbation of the bounds, all solved in ONE batched linprog call
// Everything is built on the shared batched LP solver (math/linprog.h): a sweep over thousands of knock-outs, medium
// conditions or the 2r objectives of FVA is a single call whose lanes share S, not a host loop of LPs.

#include "pluss/subsp/math/linprog.h"
#include "pluss/subsp/natsci/biology/dynamics.h"

#include <optional>
#include <string>
#include <vector>

namespace pluss::subsp::natsci::biology::metabolism {

namespace linprog = ::pluss::subsp::math::linprog;

// Constraint-based analysis wants EXACT vertex answers (alternate optima, pinned objective floors and fixed fluxes make interior-point
// results accuracy-limited, ~1e-4 to 1e-2 at genome scale), so the default solver is the simplex; sweeps are warm-started from the
// wild-type optimal basis (bound changes -> dual simplex, objective changes -> primal simplex). Pass interior-point options to trade
// exactness for batched throughput on small models.
inline linprog::Options exact_options() {
  linprog::Options o;
  o.method = linprog::Method::Simplex;
  return o;
}

struct Network final {
  Series stoichiometry = pluss::series::undefined(); // [m metabolites, r reactions]
  Series lower = pluss::series::undefined();          // [r] or [..., r]
  Series upper = pluss::series::undefined();          // [r] or [..., r]
  std::vector<std::string> reactions{};               // optional names
  std::vector<std::string> metabolites{};
};

struct Solution final {
  Series flux = pluss::series::undefined();         // [..., r]
  Series objective = pluss::series::undefined();    // [...]  (in the maximised sense)
  Series status = pluss::series::undefined();       // [...]  linprog::Status codes (0 Optimal, 4 NearOptimal both succeed)
  Series shadow_price = pluss::series::undefined(); // [..., m]  d objective / d (metabolite balance), from the LP dual
  std::vector<std::vector<std::int64_t>> basis{};  // simplex only: optimal basis per lane (warm start for related LPs)
  std::vector<std::vector<char>> at_upper{};
};

inline void validate(const Network &net) {
  dynamics::require(net.stoichiometry.defined() && net.stoichiometry.dim() == 2, "metabolism: stoichiometry must be [metabolites, reactions]");
  const std::int64_t r = net.stoichiometry.size(1);
  dynamics::require(net.lower.defined() && net.upper.defined() && net.lower.size(-1) == r && net.upper.size(-1) == r,
                    "metabolism: bounds must have one entry per reaction");
  dynamics::require(net.lower.le(net.upper).all().item<bool>(), "metabolism: lower bound exceeds upper bound");
}

namespace detail {
// Optimal or NearOptimal (accuracy-limited but certified, see linprog::Status)
inline Series succeeded(const Series &status) {
  return status.eq(static_cast<double>(linprog::Status::Optimal)).logical_or(status.eq(static_cast<double>(linprog::Status::NearOptimal)));
}
inline Series zero_rows(const Network &net, const Series &like_c) { // [..., m] zeros matching the lanes of c
  std::vector<std::int64_t> shape;
  for (std::int64_t i = 0; i + 1 < like_c.dim(); ++i)
    shape.push_back(like_c.size(i));
  shape.push_back(net.stoichiometry.size(0));
  return pluss::series::zeros(shape, Config{}.with_dtype(like_c.scalar_type()).with_platform(like_c.platform()));
}
} // namespace detail

// Maximise c^T v. `objective` is [r] or [..., r] (a sweep of objectives is a batch of lanes); bounds may also
// carry lane axes ([..., r]).
inline Solution flux_balance(const Network &net, const Series &objective, const linprog::Options &opt = exact_options(),
                             const Solution *warm = nullptr) {
  validate(net);
  const Series c = objective.to(net.stoichiometry.scalar_type()).to(net.stoichiometry.platform());
  linprog::Problem p;
  p.c = c.neg();
  p.a_eq = net.stoichiometry;
  p.b_eq = detail::zero_rows(net, c);
  p.lower = net.lower;
  p.upper = net.upper;
  if (warm != nullptr && !warm->basis.empty() && !warm->basis[0].empty()) { // same standard form: only bounds / costs differ
    p.warm_basis = warm->basis[0];
    p.warm_at_upper = warm->at_upper[0];
  }
  const auto r = linprog::solve(p, opt);
  return {r.x, r.objective.neg(), r.status, r.dual_eq.neg(), r.basis, r.at_upper};
}

struct Variability final {
  Series minimum = pluss::series::undefined(); // [r]
  Series maximum = pluss::series::undefined(); // [r]
  Series optimum = pluss::series::undefined(); // scalar objective z*
};

// Flux variability analysis: all 2r single-reaction LPs go through ONE batched solve (lanes = +/- unit vectors).
// `fraction` in [0, 1] of the optimal objective is enforced as c^T v >= fraction z*.
inline Variability flux_variability(const Network &net, const Series &objective, double fraction = 1.0,
                                    const linprog::Options &opt = exact_options()) {
  validate(net);
  dynamics::require(net.lower.dim() == 1 && net.upper.dim() == 1, "flux_variability: bounds must be [r]");
  dynamics::require(fraction >= 0.0 && fraction <= 1.0, "flux_variability: fraction must lie in [0, 1]");
  const std::int64_t r = net.stoichiometry.size(1);
  const Config cfg = Config{}.with_dtype(net.stoichiometry.scalar_type()).with_platform(net.stoichiometry.platform());
  const Series c = objective.to(cfg.dtype).to(cfg.platform);
  const auto wt = flux_balance(net, c, opt);
  dynamics::require(detail::succeeded(wt.status).item<bool>(), "flux_variability: the reference optimum was not found");
  const Series zstar = wt.objective;
  const Series e = pluss::series::eye(r, cfg);
  // lanes 0..r-1 minimise v_j (c = +e_j), lanes r..2r-1 maximise v_j (c = -e_j)
  const Series lane_c = e.unite(std::vector<Series>{e.neg()}, 0); // [2r, r]
  linprog::Problem p;
  p.c = lane_c;
  p.a_eq = net.stoichiometry;
  p.b_eq = pluss::series::zeros({2 * r, net.stoichiometry.size(0)}, cfg);
  p.a_ub = c.neg().unsqueeze(0); // -c^T v <= -f z*   (one shared row)
  p.b_ub = zstar.mul(-fraction).reshape({1}).unsqueeze(0).expand({2 * r, 1}).contiguous();
  p.lower = net.lower;
  p.upper = net.upper;
  if (!wt.basis.empty() && !wt.basis[0].empty()) { // the wild-type optimum stays feasible with the extra floor row: its slack enters the basis
    std::vector<std::int64_t> basis = wt.basis[0];
    std::vector<char> upper = wt.at_upper[0];
    basis.push_back(static_cast<std::int64_t>(upper.size()));
    upper.push_back(0);
    p.warm_basis = std::move(basis);
    p.warm_at_upper = std::move(upper);
  }
  const auto sol = linprog::solve(p, opt);
  dynamics::require(detail::succeeded(sol.status).all().item<bool>(), "flux_variability: a variability LP did not reach optimality");
  const Series v = sol.x;                                  // [2r, r]
  const Series diag = e;                                   // pick v_j from lane j
  const Series mn = v.slice(0, 0, r, 1).mul(diag).sum(-1, false);
  const Series mx = v.slice(0, r, 2 * r, 1).mul(diag).sum(-1, false);
  return {mn, mx, zstar};
}

struct Knockout final {
  Series objective = pluss::series::undefined(); // [K] optimal objective with each perturbation (0 if the lane is infeasible)
  Series status = pluss::series::undefined();    // [K]
  Series ratio = pluss::series::undefined();     // [K] objective / wild-type objective
};

// Knock out sets of reactions: `mask` [K, r] (1 = that reaction is forced to zero flux in lane k). All K LPs are one
// batched call; bounds become lane-dependent while S stays shared.
inline Knockout knockouts(const Network &net, const Series &objective, const Series &mask, const linprog::Options &opt = exact_options()) {
  validate(net);
  const std::int64_t r = net.stoichiometry.size(1);
  dynamics::require(mask.dim() == 2 && mask.size(1) == r, "knockouts: mask must be [K, reactions]");
  const Series keep = mask.to(net.stoichiometry.scalar_type()).to(net.stoichiometry.platform()).neg().add(1.0);
  const Series lo = net.lower.unsqueeze(0).mul(keep), hi = net.upper.unsqueeze(0).mul(keep); // killed -> [0, 0]
  Network lane = net;
  lane.lower = lo;
  lane.upper = hi;
  const Series c = objective.to(net.stoichiometry.scalar_type()).to(net.stoichiometry.platform());
  const auto wt = flux_balance(net, c, opt);
  const auto ko = flux_balance(lane, c.unsqueeze(0).expand({mask.size(0), r}).contiguous(), opt, &wt);
  // an infeasible / non-optimal lane (e.g. a forced-flux reaction knocked out) is reported as lethal: objective 0
  const Series obj = detail::succeeded(ko.status).where(ko.objective, ko.objective.mul(0.0));
  return {obj, ko.status, obj.div(wt.objective)};
}

// Every single-reaction knock-out (K = r lanes).
inline Knockout single_knockouts(const Network &net, const Series &objective, const linprog::Options &opt = exact_options()) {
  const std::int64_t r = net.stoichiometry.size(1);
  const Config cfg = Config{}.with_dtype(net.stoichiometry.scalar_type()).with_platform(net.stoichiometry.platform());
  return knockouts(net, objective, pluss::series::eye(r, cfg), opt);
}

// Robustness / phenotype sweep: optimum as reaction `index` is clamped to each value in `values` [K]
// (e.g. the substrate uptake rate); lanes differ only in the bounds of that reaction.
inline Solution sweep_flux(const Network &net, const Series &objective, std::int64_t index, const Series &values,
                           const linprog::Options &opt = exact_options()) {
  validate(net);
  const std::int64_t r = net.stoichiometry.size(1), k = values.size(0);
  dynamics::require(index >= 0 && index < r, "sweep_flux: reaction index out of range");
  const Config cfg = Config{}.with_dtype(net.stoichiometry.scalar_type()).with_platform(net.stoichiometry.platform());
  const Series sel = pluss::series::eye(r, cfg).select(0, index).unsqueeze(0); // [1, r]
  const Series v = values.to(cfg.dtype).to(cfg.platform).unsqueeze(-1);          // [K, 1]
  const Series keep = sel.neg().add(1.0);
  Network lane = net;
  lane.lower = net.lower.unsqueeze(0).mul(keep).add(v.mul(sel));
  lane.upper = net.upper.unsqueeze(0).mul(keep).add(v.mul(sel));
  const Series c = objective.to(cfg.dtype).to(cfg.platform);
  const auto wt = flux_balance(net, c, opt); // reference optimum: its basis warm-starts every sweep point
  return flux_balance(lane, c.unsqueeze(0).expand({k, r}).contiguous(), opt, &wt);
}

} // namespace pluss::subsp::natsci::biology::metabolism
