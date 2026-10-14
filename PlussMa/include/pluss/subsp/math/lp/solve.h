#pragma once

// Facade: presolve -> sparse simplex -> postsolve, returning primal values, row duals, reduced costs and a basis of the ORIGINAL model.
//
// When presolve removes anything and duals / a basis are requested, the simplex is warm-started on the original model from the
// basis expanded from the reduced solution (needs 0 to a few pivots), so every reported quantity belongs to the original problem.

#include "pluss/subsp/math/lp/presolve.h"
#include "pluss/subsp/math/lp/simplex.h"

#include <algorithm>
#include <chrono>

namespace pluss::subsp::math::lp {

struct LpOptions final {
  SimplexOptions simplex{};
  PresolveOptions presolve{};
  bool want_duals = true; // false: skip the postsolve warm start (primal solution + objective only)
  bool certify = true;    // presolve infeasible / unbounded verdicts are re-derived on the unreduced model to return a Farkas proof / ray
};

inline LpSolution solve(const Model &model, const LpOptions &opt = {}) {
  model.validate();
  if (!opt.presolve.enabled) {
    Simplex sx(model, opt.simplex);
    return sx.solve();
  }
  const auto pre = presolve(model, opt.presolve);
  LpSolution out;
  if (pre.status == SolveStatus::Infeasible || pre.status == SolveStatus::Unbounded || pre.status == SolveStatus::InfeasibleOrUnbounded) {
    // presolve cannot tell a cost-unbounded direction from infeasibility reliably in the second case: re-solve unreduced to classify
    if (pre.status == SolveStatus::InfeasibleOrUnbounded || opt.certify) {
      Simplex sx(model, opt.simplex);
      return sx.solve();
    }
    out.status = pre.status;
    return out;
  }
  if (pre.removed_rows == 0 && pre.removed_cols == 0) {
    Simplex sx(model, opt.simplex);
    return sx.solve();
  }
  LpSolution red;
  double reduced_seconds = 0.0;
  if (pre.reduced.cols == 0 || pre.reduced.rows == 0 && pre.reduced.cols == 0) {
    red.status = SolveStatus::Optimal;
    red.x.clear();
    red.objective = pre.reduced.offset;
  } else {
    const auto t_red0 = std::chrono::steady_clock::now();
    Simplex sx(pre.reduced, opt.simplex);
    red = sx.solve();
    reduced_seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - t_red0).count();
  }
  if (red.status != SolveStatus::Optimal) {
    out.status = red.status;
    out.iterations = red.iterations;
    if (red.status == SolveStatus::Unbounded && !red.ray.empty()) {
      out.ray.assign(static_cast<std::size_t>(model.cols), 0.0);
      for (std::size_t k = 0; k < pre.col_map.size(); ++k)
        out.ray[static_cast<std::size_t>(pre.col_map[k])] = red.ray[k];
    }
    return out;
  }
  out = red;
  out.x = pre.expand(red.x);
  if (!opt.want_duals) {
    out.row_activity.assign(static_cast<std::size_t>(model.rows), 0.0);
    model.a.multiply_add(1.0, out.x, out.row_activity);
    double obj = model.offset;
    for (std::int32_t j = 0; j < model.cols; ++j)
      obj += model.cost[static_cast<std::size_t>(j)] * out.x[static_cast<std::size_t>(j)];
    out.objective = obj;
    out.row_dual.clear();
    out.reduced.clear();
    out.basis.clear();
    return out;
  }
  // duals and basis of the original model: warm start from the expanded basis
  Simplex sx(model, opt.simplex);
  if (!red.basis.empty() && sx.set_basis(pre.expand_basis(red.basis))) {
    // The expanded point is primal feasible and optimal for everything the presolve kept, so only the removed rows / columns can
    // leave dual infeasibilities: a primal simplex from this basis needs a handful of pivots, whereas the dual simplex would start
    // with a full dual phase I.
    sx.set_iteration_limit(100000);
    sx.set_time_limit(std::max(1.0, 0.25 * reduced_seconds)); // cheap by design; otherwise the dual route below is faster
    auto full = sx.resolve_primal();
    if (full.status == SolveStatus::Optimal) { // the primal route trusts its starting point: verify objective and primal feasibility
      const double scale = 1.0 + std::abs(red.objective);
      bool ok = std::abs(full.objective - red.objective) <= 1e-7 * scale;
      if (ok) {
        std::vector<double> act(static_cast<std::size_t>(model.rows), 0.0);
        model.a.multiply_add(1.0, full.x, act);
        for (std::int32_t i = 0; i < model.rows && ok; ++i)
          ok = act[static_cast<std::size_t>(i)] >= model.row_lo[static_cast<std::size_t>(i)] - 1e-6 * (1.0 + std::abs(model.row_lo[static_cast<std::size_t>(i)])) &&
               act[static_cast<std::size_t>(i)] <= model.row_hi[static_cast<std::size_t>(i)] + 1e-6 * (1.0 + std::abs(model.row_hi[static_cast<std::size_t>(i)]));
        for (std::int32_t j = 0; j < model.cols && ok; ++j)
          ok = full.x[static_cast<std::size_t>(j)] >= model.col_lo[static_cast<std::size_t>(j)] - 1e-6 * (1.0 + std::abs(model.col_lo[static_cast<std::size_t>(j)])) &&
               full.x[static_cast<std::size_t>(j)] <= model.col_hi[static_cast<std::size_t>(j)] + 1e-6 * (1.0 + std::abs(model.col_hi[static_cast<std::size_t>(j)]));
      }
      if (!ok)
        full.status = SolveStatus::IterationLimit; // force the fallback below
    }
    if (full.status != SolveStatus::Optimal) {
      Simplex again(model, opt.simplex);
      if (again.set_basis(pre.expand_basis(red.basis)))
        full = again.solve();
      else
        full = Simplex(model, opt.simplex).solve();
    }
    full.iterations += red.iterations;
    return full;
  }
  Simplex cold(model, opt.simplex);
  return cold.solve();
}

} // namespace pluss::subsp::math::lp
