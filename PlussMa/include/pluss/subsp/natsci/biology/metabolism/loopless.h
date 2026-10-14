#pragma once

// Thermodynamically infeasible loop removal (CycleFreeFlux, Desouki, Jarre, Gelius-Dietrich & Lercher, Bioinformatics 31:2159, 2015)
// and loopless flux variability analysis following COBRApy's `loopless_fva_iter` (cobra 0.29+, loopless="cycleFreeFlux").
//
//   cycle_free(v):  boundary reactions fixed at their value in v; every internal reaction is restricted to the sign of v and to
//                   |flux| <= |v|; the objective is held at its optimum; minimise sum |v_internal|.
//   Every stage below is a batched LP over lanes (one lane per starting flux vector): the 2r single-reaction FVA solutions are
//   cleaned in one call, the lanes that still contain a loop through the target reaction are re-solved in two more batched calls.

#include "pluss/subsp/math/milp.h"
#include "pluss/subsp/natsci/biology/metabolism/fba.h"

#include <cmath>
#include <optional>
#include <utility>
#include <vector>

namespace pluss::subsp::natsci::biology::metabolism {

namespace math = ::pluss::subsp::math;

// Boundary (exchange / demand / sink) reactions: exactly one non-zero stoichiometric coefficient. Returns [r] in {0,1}.
inline Series boundary_reactions(const Network &net) {
  return net.stoichiometry.ne(0.0).to(net.stoichiometry.scalar_type()).sum(0, false).eq(1.0).to(net.stoichiometry.scalar_type());
}

namespace detail {
// Cycle-free box around a starting flux v [L, r]: boundary reactions pinned to v, internal reactions kept on the sign of v
// and below |v|. The interior-point LP returns fluxes that satisfy S v = 0 only to ~1e-7, so the box is widened by that
// tolerance (an exact simplex, as in COBRApy, needs no slack); the widening is far below the 1e-5 accuracy claimed.
// Least-squares projection of fluxes v [L, r] onto {S v = 0}: v - S^T (S S^T)^+ S v. Interior-point fluxes satisfy the steady
// state only to ~1e-6; after the projection it holds to round-off, so the cycle-free box below always contains a feasible point.
inline Series project_steady_state(const Network &net, const Series &v) {
  const Series s = net.stoichiometry;
  const Series gram_pinv = pluss::subsp::linearalg::pinverse(s.matmul(s.transpose(0, 1)), 1e-12);
  return v.sub(v.matmul(s.transpose(0, 1)).matmul(gram_pinv).matmul(s));
}
struct CycleBox final {
  Series lower, upper;
};
inline CycleBox cycle_box(const Network &net, const Series &v) {
  const auto dt = net.stoichiometry.scalar_type();
  const Series boundary = boundary_reactions(net).unsqueeze(0);
  const Series pos = v.ge(0.0); // bool mask (where() needs a bool condition)
  const Series zero = v.mul(0.0);
  const Series lo = net.lower.unsqueeze(0).add(zero), hi = net.upper.unsqueeze(0).add(zero);
  const Series slack = v.abs().add(1.0).mul(1e-7);
  Series int_lo = pos.where(lo.clamp_min(0.0), lo.maximum(v.sub(slack)));
  Series int_hi = pos.where(hi.minimum(v.add(slack)), hi.clamp_max(0.0));
  const Series is_b = boundary.add(zero).gt(0.5);
  Series lower = is_b.where(v.sub(slack), int_lo), upper = is_b.where(v.add(slack), int_hi);
  // always contain the (projected) starting point itself: the LP is then feasible by construction
  return {lower.minimum(v), upper.maximum(v)};
}
// Cycle-free LP for starting fluxes `v` [L, r]; returns the cleaned fluxes [L, r]. `extra_lower/upper` optionally
// further restrict the internal bounds (used by the second loopless-FVA stage).
inline Series cycle_free_lp(const Network &net, const Series &v_in, const Series &objective, const Series &objective_floor,
                            const linprog::Options &opt) {
  const Series v = project_steady_state(net, v_in);
  const std::int64_t L = v.size(0), r = net.stoichiometry.size(1);
  const auto dt = net.stoichiometry.scalar_type();
  const Series boundary = boundary_reactions(net).unsqueeze(0);                  // [1, r]
  const Series pos = v.ge(0.0).to(dt);
  const auto box = cycle_box(net, v);
  const Series lower = box.lower, upper = box.upper;
  linprog::Problem p;
  p.c = pos.mul(2.0).sub(1.0).mul(boundary.neg().add(1.0)); // +1 for forward-signed internal, -1 for reverse-signed, 0 boundary
  p.a_eq = net.stoichiometry;
  p.b_eq = pluss::series::zeros({L, net.stoichiometry.size(0)}, Config{}.with_dtype(dt).with_platform(v.platform()));
  p.a_ub = objective.neg().unsqueeze(0); // objective(v) >= floor
  p.b_ub = objective_floor.neg().reshape({1, 1}).expand({L, 1}).contiguous();
  p.lower = lower;
  p.upper = upper;
  (void)r;
  const auto sol = linprog::solve(p, opt);
  // keep the original flux for lanes where the cleaning LP did not succeed
  return detail::succeeded(sol.status).unsqueeze(-1).where(sol.x, v_in);
}
} // namespace detail

// ---- exact loop-law MILP (ll-COBRA; Schellenberger, Lewis & Palsson, Biophys. J. 100:544, 2011) ------------------------------
// Variables [v (r), a (internal, binary: reaction runs forward), G (internal, "energy")]:
//   S v = 0;  -M(1-a_i) <= v_i <= M a_i;  -M a_i + (1-a_i) <= G_i <= -a_i + M(1-a_i);  N_int^T G = 0
// where N_int spans the null space of the internal stoichiometry (the internal cycles). Exact: a flux is loopless iff such
// (a, G) exist. Used for the lanes the cheap cycle-free screen cannot certify.
struct LoopMilp final {
  linprog::Problem lp;     // c is filled per use (only the first r entries are non-zero)
  Series integer = pluss::series::undefined();
  std::int64_t reactions{0};
  std::vector<std::int64_t> internal;
};

// `flux_min` / `flux_max` ([r], optional) are valid bounds on every flux under the problem's other constraints (e.g. FVA ranges);
// they give per-reaction big-M values, which tighten the LP relaxation enormously compared with a global M = max bound.
inline LoopMilp loop_law_problem(const Network &net, const std::optional<Series> &flux_min = std::nullopt,
                                 const std::optional<Series> &flux_max = std::nullopt) {
  const auto dt = net.stoichiometry.scalar_type();
  const Config cfg = Config{}.with_dtype(dt).with_platform(net.stoichiometry.platform());
  const std::int64_t m = net.stoichiometry.size(0), r = net.stoichiometry.size(1);
  const auto boundary = boundary_reactions(net).to_vec<double>();
  std::vector<std::int64_t> internal;
  for (std::int64_t j = 0; j < r; ++j)
    if (boundary[static_cast<std::size_t>(j)] < 0.5)
      internal.push_back(j);
  const std::int64_t ri_all = static_cast<std::int64_t>(internal.size());
  auto selector = [&](const std::vector<std::int64_t> &which) {
    std::vector<double> sel(which.size() * static_cast<std::size_t>(r), 0.0);
    for (std::size_t k = 0; k < which.size(); ++k)
      sel[k * static_cast<std::size_t>(r) + static_cast<std::size_t>(which[k])] = 1.0;
    return pluss::make_series(sel, {static_cast<std::int64_t>(which.size()), r}).to(dt).to(cfg.platform);
  };
  // null space of the internal stoichiometry = the internal cycles
  const Series s_int = net.stoichiometry.matmul(selector(internal).transpose(0, 1)); // [m, ri_all]
  const auto sv = pluss::subsp::linearalg::svd_s(s_int, true).to_vec<double>();
  const Series vh = pluss::subsp::linearalg::svd_vh(s_int, true);                    // [ri_all, ri_all]
  std::int64_t rank = 0;
  for (double x : sv)
    if (x > 1e-9 * sv[0])
      ++rank;
  LoopMilp out;
  out.reactions = r;
  const std::int64_t k_null = ri_all - rank;
  if (k_null <= 0) { // no internal cycles at all: every feasible flux is loopless
    out.internal.clear();
    return out;
  }
  const Series null_all = vh.slice(0, rank, ri_all, 1); // [k, ri_all]
  // A reaction outside the support of the cycle space has a zero coefficient in N^T G = 0, so its direction indicator and
  // energy are unconstrained and can always be chosen consistently: only the support needs binaries.
  const auto support_host = null_all.abs().amax(0, false).to_vec<double>();
  std::vector<std::int64_t> cyc_pos; // positions within `internal`
  for (std::int64_t i = 0; i < ri_all; ++i)
    if (support_host[static_cast<std::size_t>(i)] > 1e-9)
      cyc_pos.push_back(i);
  std::vector<double> pos_d(cyc_pos.begin(), cyc_pos.end());
  const std::int64_t ri = static_cast<std::int64_t>(cyc_pos.size());
  const Series null_basis = null_all.index_select(1, pluss::make_series(pos_d, {ri}).to(pluss::ScalarType::Int64).to(cfg.platform)); // [k, ri]
  for (const auto pos : cyc_pos)
    out.internal.push_back(internal[static_cast<std::size_t>(pos)]);
  const Series pick = selector(out.internal); // [ri, r]
  const double big_m = std::max(net.lower.abs().max().item<double>(), net.upper.abs().max().item<double>());
  const Series hi_v = flux_max ? flux_max->minimum(net.upper) : net.upper, lo_v = flux_min ? flux_min->maximum(net.lower) : net.lower;
  const Series mf = hi_v.clamp_min(0.0).add(1e-6).matmul(pick.transpose(0, 1)), mr = lo_v.neg().clamp_min(0.0).add(1e-6).matmul(pick.transpose(0, 1));
  const std::int64_t nv = r + 2 * ri;
  const Series eye = pluss::series::eye(ri, cfg), zer_ri_r = pluss::series::zeros({ri, r}, cfg), zer_ri_ri = pluss::series::zeros({ri, ri}, cfg);
  const Series row1 = pick.unite(std::vector<Series>{eye.mul(mf.neg().unsqueeze(-1)), zer_ri_ri}, 1);      //  v - Mf a <= 0
  const Series row2 = pick.neg().unite(std::vector<Series>{eye.mul(mr.unsqueeze(-1)), zer_ri_ri}, 1);      // -v + Mr a <= Mr
  const Series row3 = zer_ri_r.unite(std::vector<Series>{eye.mul(big_m + 1.0), eye}, 1);                    //  G + (M+1) a <= M
  const Series row4 = zer_ri_r.unite(std::vector<Series>{eye.mul(-(big_m + 1.0)), eye.neg()}, 1);           // -G - (M+1) a <= -1
  const Series zero_ri = pluss::series::zeros({ri}, cfg);
  out.lp.a_ub = row1.unite(std::vector<Series>{row2, row3, row4}, 0);
  out.lp.b_ub = zero_ri.unite(std::vector<Series>{mr, zero_ri.add(big_m), zero_ri.add(-1.0)}, 0);
  const Series ae1 = net.stoichiometry.unite(std::vector<Series>{pluss::series::zeros({m, 2 * ri}, cfg)}, 1);
  const Series ae2 = pluss::series::zeros({null_basis.size(0), r + ri}, cfg).unite(std::vector<Series>{null_basis}, 1);
  out.lp.a_eq = ae1.unite(std::vector<Series>{ae2}, 0);
  out.lp.b_eq = pluss::series::zeros({m + null_basis.size(0)}, cfg);
  out.lp.lower = net.lower.unite(std::vector<Series>{zero_ri, zero_ri.add(-(big_m + 1.0))}, 0);
  out.lp.upper = net.upper.unite(std::vector<Series>{pluss::series::ones({ri}, cfg), zero_ri.add(big_m + 1.0)}, 0);
  out.lp.c = pluss::series::zeros({nv}, cfg);
  out.integer = pluss::series::zeros({r}, cfg).unite(std::vector<Series>{pluss::series::ones({ri}, cfg), zero_ri}, 0);
  return out;
}

// Exact loopless flux range of one reaction by the loop-law MILP (slow reference; use `loopless_flux_variability` for sweeps).
// Returns {min, max} over all fluxes with c^T v >= fraction z* that admit an energy assignment (no thermodynamically infeasible loop).
inline std::pair<double, double> loopless_range_exact(const Network &net, const Series &objective, std::int64_t reaction,
                                                      double fraction = 1.0, math::milp::Options mopt = {}) {
  validate(net);
  const auto dt = net.stoichiometry.scalar_type();
  const Config cfg = Config{}.with_dtype(dt).with_platform(net.stoichiometry.platform());
  const std::int64_t r = net.stoichiometry.size(1);
  dynamics::require(reaction >= 0 && reaction < r, "loopless_range_exact: reaction index out of range");
  const auto plain = flux_variability(net, objective, fraction);
  auto loop = loop_law_problem(net, plain.minimum, plain.maximum);
  if (loop.internal.empty()) // no internal cycles: the plain range is already loopless
    return {plain.minimum.select(0, reaction).item<double>(), plain.maximum.select(0, reaction).item<double>()};
  const std::int64_t nv = loop.lp.c.size(0);
  const Series c = objective.to(dt).to(cfg.platform);
  const Series growth_row = c.neg().unite(std::vector<Series>{pluss::series::zeros({nv - r}, cfg)}, 0).unsqueeze(0);
  loop.lp.a_ub = loop.lp.a_ub->unite(std::vector<Series>{growth_row}, 0);
  loop.lp.b_ub = loop.lp.b_ub->unite(std::vector<Series>{pluss::series::zeros({1}, cfg).add(-plain.optimum.item<double>() * fraction * (1.0 - 1e-9))}, 0);
  mopt.lp.method = linprog::Method::Simplex;
  double ends[2];
  for (int side = 0; side < 2; ++side) {
    std::vector<double> cost(static_cast<std::size_t>(nv), 0.0);
    cost[static_cast<std::size_t>(reaction)] = side == 0 ? 1.0 : -1.0;
    math::milp::Problem mp;
    mp.lp = loop.lp;
    mp.lp.c = pluss::make_series(cost, {nv}).to(dt).to(cfg.platform);
    mp.integer = loop.integer;
    const auto sol = math::milp::solve(mp, mopt);
    dynamics::require(sol.status == math::milp::Status::Optimal, "loopless_range_exact: the loop-law MILP did not reach optimality");
    ends[side] = (side == 0 ? 1.0 : -1.0) * sol.objective.item<double>();
  }
  return {ends[0], ends[1]};
}

// Remove loops from one (or a batch of) flux distribution(s) [.., r]; exchange fluxes and flux directions are preserved.
inline Series loopless_solution(const Network &net, const Series &objective, const Series &fluxes, const linprog::Options &opt_in = {}) {
  validate(net);
  linprog::Options opt = opt_in;
  opt.method = linprog::Method::Simplex; // vertex solutions: loop removal needs an exact vertex of the optimal face
  const Series v = fluxes.dim() == 1 ? fluxes.unsqueeze(0) : fluxes;
  const Series obj = objective.to(v.scalar_type()).to(v.platform());
  const Series floor = v.mul(obj).sum(-1, true).amin(0, false); // the smallest objective among the lanes (they share an optimum)
  const double tol = 1e-7;
  const Series cleaned = detail::cycle_free_lp(net, v, obj, floor.sub(tol * (1.0 + std::abs(floor.item<double>()))), opt);
  return fluxes.dim() == 1 ? cleaned.squeeze(0) : cleaned;
}

// Loopless flux variability (cycleFreeFlux). Same output as `flux_variability` but every range excludes thermodynamically
// infeasible loops.
inline Variability loopless_flux_variability(const Network &net, const Series &objective, double fraction = 1.0,
                                             const linprog::Options &opt_in = {}) {
  validate(net);
  linprog::Options opt = opt_in;
  opt.method = linprog::Method::Simplex; // CycleFreeFlux relies on vertex (not analytic-centre) solutions
  const std::int64_t r = net.stoichiometry.size(1), m = net.stoichiometry.size(0);
  const auto dt = net.stoichiometry.scalar_type();
  const Config cfg = Config{}.with_dtype(dt).with_platform(net.stoichiometry.platform());
  const Series c = objective.to(dt).to(cfg.platform);
  const auto wt = flux_balance(net, c, opt);
  dynamics::require(detail::succeeded(wt.status).item<bool>(), "loopless_flux_variability: reference optimum not found");
  const double zstar = wt.objective.item<double>();
  const double floor_value = fraction * zstar;
  const Series e = pluss::series::eye(r, cfg);
  const Series lane_c = e.unite(std::vector<Series>{e.neg()}, 0); // [2r, r]: rows 0..r-1 minimise v_j, r..2r-1 maximise
  const Series boundary = boundary_reactions(net);
  const double cutoff = 1e-7;

  auto stage_a = [&]() { // original FVA LPs, keeping the full flux vectors
    linprog::Problem p;
    p.c = lane_c;
    p.a_eq = net.stoichiometry;
    p.b_eq = pluss::series::zeros({2 * r, m}, cfg);
    p.a_ub = c.neg().unsqueeze(0);
    p.b_ub = pluss::series::zeros({2 * r, 1}, cfg).add(-floor_value);
    p.lower = net.lower;
    p.upper = net.upper;
    if (!wt.basis.empty() && !wt.basis[0].empty()) { // warm start from the wild-type optimum (+ the floor row's slack)
      std::vector<std::int64_t> basis = wt.basis[0];
      std::vector<char> upper = wt.at_upper[0];
      basis.push_back(static_cast<std::int64_t>(upper.size()));
      upper.push_back(0);
      p.warm_basis = std::move(basis);
      p.warm_at_upper = std::move(upper);
    }
    return linprog::solve(p, opt);
  };
  const auto fva = stage_a();
  dynamics::require(detail::succeeded(fva.status).all().item<bool>(), "loopless_flux_variability: an FVA LP failed");
  const Series v = detail::project_steady_state(net, fva.x); // [2r, r]  (exact steady state)
  const Series current = v.mul(lane_c.abs()).sum(-1, false); // flux of the target reaction in each lane
  const Series ll = detail::cycle_free_lp(net, v, c, pluss::series::zeros({}, cfg).add(floor_value - 1e-7 * (1.0 + std::abs(floor_value))), opt);
  const Series ll_target = ll.mul(lane_c.abs()).sum(-1, false);
  Series result = current;
  // lanes whose target flux changed under cleaning (and that are not boundary reactions) still carry a loop through the target
  const Series target_boundary = boundary.unsqueeze(0).add(pluss::series::zeros({2 * r, 1}, cfg)).mul(lane_c.abs()).sum(-1, false).gt(0.5);
  const Series needs = ll_target.sub(current).abs().ge(cutoff).logical_and(target_boundary.logical_not());
  const auto need_host = needs.to(dt).to_vec<double>();
  std::vector<double> idx_host;
  for (std::size_t i = 0; i < need_host.size(); ++i)
    if (need_host[i] > 0.5)
      idx_host.push_back(static_cast<double>(i));
  if (idx_host.empty())
    return {result.slice(0, 0, r, 1), result.slice(0, r, 2 * r, 1), wt.objective};
  const std::int64_t S = static_cast<std::int64_t>(idx_host.size());
  const Series idx = pluss::make_series(idx_host, {S}).to(pluss::ScalarType::Int64);
  const Series v_s = v.index_select(0, idx), cur_s = current.index_select(0, idx), c_s = lane_c.index_select(0, idx), ll_s = ll.index_select(0, idx);
  // stage C: restrict to the cycle-free box of the FVA solution, fix the target at its FVA value, minimise the same L1 norm
  const Series pos = v_s.ge(0.0).to(dt), zero_s = v_s.mul(0.0);
  const Series lo = net.lower.unsqueeze(0).add(zero_s), hi = net.upper.unsqueeze(0).add(zero_s);
  const auto box_s = detail::cycle_box(net, v_s);
  Series lowerC = box_s.lower, upperC = box_s.upper;
  const Series target_sel = c_s.abs().gt(0.5);
  const Series cur_slack = cur_s.abs().add(1.0).mul(1e-7).unsqueeze(-1);
  lowerC = target_sel.where(cur_s.unsqueeze(-1).sub(cur_slack).add(zero_s), lowerC);
  upperC = target_sel.where(cur_s.unsqueeze(-1).add(cur_slack).add(zero_s), upperC);
  linprog::Problem pc;
  pc.c = pos.mul(2.0).sub(1.0).mul(boundary.neg().add(1.0).unsqueeze(0));
  pc.a_eq = net.stoichiometry;
  pc.b_eq = pluss::series::zeros({S, m}, cfg);
  pc.a_ub = c.neg().unsqueeze(0);
  pc.b_ub = pluss::series::zeros({S, 1}, cfg).add(-(floor_value - 1e-7 * (1.0 + std::abs(floor_value))));
  pc.lower = lowerC;
  pc.upper = upperC;
  const auto almost = linprog::solve(pc, opt);
  // stage D: forbid the reactions that are zero in the cleaned solution but non-zero in the almost-clean one
  const Series forbid = ll_s.abs().lt(cutoff).logical_and(almost.x.abs().gt(cutoff));
  const Series lowerD = forbid.where(lo.clamp_min(0.0), lo), upperD = forbid.where(hi.clamp_max(0.0), hi);
  linprog::Problem pd;
  pd.c = c_s;
  pd.a_eq = net.stoichiometry;
  pd.b_eq = pluss::series::zeros({S, m}, cfg);
  pd.a_ub = c.neg().unsqueeze(0);
  pd.b_ub = pluss::series::zeros({S, 1}, cfg).add(-floor_value);
  pd.lower = lowerD;
  pd.upper = upperD;
  const auto best = linprog::solve(pd, opt);
  const Series best_target = best.x.mul(c_s.abs()).sum(-1, false);
  const Series fixed_val = detail::succeeded(best.status).where(best_target, cur_s);
  // scatter the re-optimised values back into their lanes (host-indexed, S entries)
  const auto fv = fixed_val.to_vec<double>();
  std::vector<double> rh = result.to_vec<double>();
  for (std::int64_t k = 0; k < S; ++k)
    rh[static_cast<std::size_t>(idx_host[static_cast<std::size_t>(k)])] = fv[static_cast<std::size_t>(k)];
  result = pluss::make_series(rh, {2 * r}).to(dt).to(cfg.platform);
  return {result.slice(0, 0, r, 1), result.slice(0, r, 2 * r, 1), wt.objective};
}

} // namespace pluss::subsp::natsci::biology::metabolism
