#pragma once

// Facade: presolve -> sparse simplex -> postsolve, returning primal values, row duals, reduced costs and a basis of the ORIGINAL model.
//
// When presolve removes anything and duals / a basis are requested, the simplex is warm-started on the original model from the
// basis expanded from the reduced solution (needs 0 to a few pivots), so every reported quantity belongs to the original problem.

#include "pluss/subsp/math/lp/presolve.h"
#include "pluss/subsp/math/lp/aggregate.h"
#include "pluss/subsp/math/lp/ipm.h"
#include "pluss/subsp/math/lp/simplex.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <thread>

namespace pluss::subsp::math::lp {

enum class LpMethod { Auto, Simplex, InteriorPoint, Race };

struct LpOptions final {
  LpMethod method = LpMethod::Auto; // Auto = Simplex (the presolve chain pays off most there); Race runs the simplex pipeline against the interior point (+ crossover) on two threads
  std::int32_t race_min_rows = 2000;       // Race falls back to the simplex alone below this many rows ...
  std::int64_t race_min_nonzeros = 20000;  // ... and this many non-zeros
  double race_ipm_delay_seconds = 2.0;     // the interior point thread only starts when the simplex is still running after this long
  SimplexOptions simplex{};
  PresolveOptions presolve{};
  AggregateOptions aggregate{}; // second presolve stage (LP only): doubleton-equation aggregation
  bool want_duals = true; // false: skip the postsolve warm start (primal solution + objective only)
  bool certify = true;    // presolve infeasible / unbounded verdicts are re-derived on the unreduced model to return a Farkas proof / ray
};


namespace lpdetail {

inline LpSolution solve_core(const Model &model, const LpOptions &opt) {
  Simplex sx(model, opt.simplex);
  return sx.solve();
}

inline LpSolution from_ipm(const Model &model, const IpmSolution &ip) {
  LpSolution out;
  out.status = ip.status;
  out.objective = ip.objective;
  out.x = ip.x;
  out.row_activity = ip.row_activity;
  out.row_dual = ip.row_dual;
  out.basis = ip.basis;
  out.iterations = ip.crossover_iterations;
  out.ray = ip.ray;
  out.farkas = ip.farkas;
  if (ip.status == SolveStatus::Optimal && ip.row_dual.size() == static_cast<std::size_t>(model.rows)) {
    out.reduced = model.cost;
    model.a.multiply_transpose_add(-1.0, out.row_dual, out.reduced); // d = c - A^T y
  }
  return out;
}

} // namespace lpdetail

// presolve -> simplex -> postsolve (no method selection)
//
// The model goes through a chain of reductions: presolve.h (static, MILP-safe), then aggregate.h (dynamic, LP-only doubleton
// aggregation), repeated until neither changes anything. The solution, the basis and the duals are rebuilt by unwinding the chain.
inline LpSolution solve_simplex_pipeline(const Model &model, const LpOptions &opt) {
  model.validate();
  if (model.has_huge_values()) { // sentinel infinities (1e30, 1e300, ...) -> real infinities, see sparse.h kInfThreshold
    Model canonical = model;
    canonical.canonicalize_infinities();
    return solve_simplex_pipeline(canonical, opt);
  }
  if (!opt.presolve.enabled)
    return lpdetail::solve_core(model, opt);
  struct Stage {
    bool aggregated{false};
    PresolveResult pre;
    AggregateResult agg;
  };
  std::vector<Stage> chain;
  Model current = model;
  current.integer.clear(); // the LP relaxation is solved here
  for (int round = 0; round < 6; ++round) {
    bool changed = false;
    {
      Stage st;
      st.pre = presolve(current, opt.presolve);
      if (st.pre.status == SolveStatus::Infeasible || st.pre.status == SolveStatus::Unbounded || st.pre.status == SolveStatus::InfeasibleOrUnbounded) {
        // presolve cannot tell a cost-unbounded direction from infeasibility reliably in the second case: re-solve unreduced to classify
        if (st.pre.status == SolveStatus::InfeasibleOrUnbounded || opt.certify) {
          Simplex sx(model, opt.simplex);
          return sx.solve();
        }
        LpSolution out;
        out.status = st.pre.status;
        return out;
      }
      if (st.pre.removed_rows > 0 || st.pre.removed_cols > 0) {
        current = st.pre.reduced;
        chain.push_back(std::move(st));
        changed = true;
      }
    }
    if (current.rows == 0 || current.cols == 0)
      break;
    {
      Stage st;
      st.aggregated = true;
      st.agg = aggregate(current, opt.aggregate);
      if (st.agg.status == SolveStatus::Infeasible) {
        Simplex sx(model, opt.simplex); // classify (and certify) on the unreduced model
        return sx.solve();
      }
      if (st.agg.changed()) {
        current = st.agg.reduced;
        chain.push_back(std::move(st));
        changed = true;
      }
    }
    if (!changed)
      break;
  }
  if (chain.empty())
    return lpdetail::solve_core(model, opt);
  LpSolution red;
  double reduced_seconds = 0.0;
  if (current.cols == 0 || current.rows == 0) {
    red.status = SolveStatus::Optimal; // everything was eliminated: the reduced problem is the constant offset
    red.x.assign(static_cast<std::size_t>(current.cols), 0.0);
    for (std::int32_t j = 0; j < current.cols; ++j) { // free of rows: every column sits at its cheapest bound
      const auto sj = static_cast<std::size_t>(j);
      const double c = current.cost[sj];
      double v = c > 0 ? current.col_lo[sj] : (c < 0 ? current.col_hi[sj] : (std::isinf(current.col_lo[sj]) ? (std::isinf(current.col_hi[sj]) ? 0.0 : current.col_hi[sj]) : current.col_lo[sj]));
      if (std::isinf(v)) {
        red.status = SolveStatus::Unbounded;
        break;
      }
      red.x[sj] = v;
    }
    red.objective = current.offset;
    for (std::int32_t j = 0; j < current.cols && red.status == SolveStatus::Optimal; ++j)
      red.objective += current.cost[static_cast<std::size_t>(j)] * red.x[static_cast<std::size_t>(j)];
    red.basis.assign(static_cast<std::size_t>(current.cols + current.rows), VarStatus::AtLower);
    for (std::int32_t i = 0; i < current.rows; ++i)
      red.basis[static_cast<std::size_t>(current.cols + i)] = VarStatus::Basic;
  } else {
    const auto t_red0 = std::chrono::steady_clock::now();
    red = lpdetail::solve_core(current, opt);
    reduced_seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - t_red0).count();
  }
  if (red.status == SolveStatus::Unbounded) { // a ray in the original space: classify on the unreduced model
    Simplex sx(model, opt.simplex);
    return sx.solve();
  }
  if (red.status != SolveStatus::Optimal) {
    LpSolution out;
    out.status = red.status;
    out.iterations = red.iterations;
    return out;
  }
  // ---- unwind the chain: primal point and basis of the original model
  std::vector<double> x = red.x;
  std::vector<VarStatus> basis = red.basis;
  for (std::size_t t = chain.size(); t-- > 0;) {
    const auto &st = chain[t];
    if (!basis.empty())
      basis = st.aggregated ? st.agg.expand_basis(basis, x) : st.pre.expand_basis(basis, x);
    x = st.aggregated ? st.agg.expand(x) : st.pre.expand(x);
  }
  LpSolution out = red;
  out.x = x;
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
  if (!basis.empty() && sx.set_basis(basis)) {
    // The expanded point is primal feasible and optimal for everything the reductions kept, so only the removed rows / columns can
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
      if (again.set_basis(basis))
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

// General entry point. Race: a large model (see LpOptions::race_*) runs two solvers concurrently, the first optimal / infeasible /
// unbounded verdict wins and cancels the other:
//   thread 1: presolve -> dual simplex -> postsolve (primal, duals and basis of the original model)
//   thread 2: interior point with crossover on the ORIGINAL model (presolve can densify the normal equations)
inline LpSolution solve(const Model &model, const LpOptions &opt = {}) {
  model.validate();
  if (model.has_huge_values()) {
    Model canonical = model;
    canonical.canonicalize_infinities();
    return solve(canonical, opt);
  }
  const bool large = model.rows >= opt.race_min_rows && model.a.nnz() >= opt.race_min_nonzeros;
  if (opt.method == LpMethod::Simplex || opt.method == LpMethod::Auto || (opt.method == LpMethod::Race && !large))
    return solve_simplex_pipeline(model, opt);
  const auto ipm_options = [&](const std::atomic<bool> *stop) {
    IpmOptions io;
    io.simplex = opt.simplex;
    io.simplex.stop = stop;
    io.stop = stop;
    io.time_limit_seconds = opt.simplex.time_limit_seconds;
    return io;
  };
  if (opt.method == LpMethod::InteriorPoint) {
    const auto ip = solve_ipm(model, ipm_options(opt.simplex.stop));
    return lpdetail::from_ipm(model, ip);
  }
  std::atomic<bool> stop{false};
  IpmSolution ip;
  std::thread worker([&] {
    const auto t_wait = std::chrono::steady_clock::now();
    while (!stop.load() && std::chrono::duration<double>(std::chrono::steady_clock::now() - t_wait).count() < opt.race_ipm_delay_seconds)
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
    if (stop.load())
      return;
    ip = solve_ipm(model, ipm_options(&stop));
    if (ip.status == SolveStatus::Optimal && ip.crossed_over)
      stop.store(true);
  });
  LpOptions so = opt;
  so.simplex.stop = &stop;
  LpSolution sol = solve_simplex_pipeline(model, so);
  const bool decided = sol.status == SolveStatus::Optimal || sol.status == SolveStatus::Infeasible || sol.status == SolveStatus::Unbounded;
  if (decided)
    stop.store(true);
  worker.join();
  if (decided)
    return sol;
  if (ip.status == SolveStatus::Optimal && ip.crossed_over)
    return lpdetail::from_ipm(model, ip);
  return sol;
}

} // namespace pluss::subsp::math::lp
