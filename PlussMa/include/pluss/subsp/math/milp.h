#pragma once

// Mixed-integer linear programming by branch-and-bound on the batched LP solver (math/linprog.h).
//
//   minimise c^T x   subject to  Aeq x = beq,  Aub x <= bub,  lower <= x <= upper,  x_j integer for j in `integer`
//
// The open nodes of the search tree are LANES of one batched LP call: every round pops up to `batch` nodes (best bound
// first), stacks their bound vectors as [B, n] and solves all relaxations together (matrices shared), so the work per round
// is one batched interior-point solve, not B separate LPs. Rounding the relaxations (also batched) supplies incumbents early.
//
//   Optimal      incumbent proven optimal within `gap_tolerance` (absolute + relative)
//   Infeasible   the LP relaxation (hence the MILP) is infeasible, or every node was pruned without an incumbent
//   Unbounded    the LP relaxation is unbounded (reported as such; integrality is not examined)
//   NodeLimit    `max_nodes` reached; the incumbent (if any) and `best_bound` are returned
// Integer variables must have finite lower and upper bounds (needed for a lane-uniform bound pattern and for termination).
// Accuracy inherits the interior-point LP tolerances (about 1e-8); integrality is accepted within `integer_tolerance`
// and the returned integer coordinates are rounded to exact integers.

#include "pluss/subsp/math/linprog.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <queue>
#include <stdexcept>
#include <vector>

namespace pluss::subsp::math::milp {

enum class Status : int { Optimal = 0, Infeasible = 1, Unbounded = 2, NodeLimit = 3 };

struct Options final {
  linprog::Options lp{};
  int max_nodes = 200000;
  int batch = 64;                    // open nodes solved per round
  double integer_tolerance = 1e-6;
  double gap_tolerance = 1e-7;       // absolute; also applied relatively: gap <= tol * (1 + |incumbent|)
};

struct Problem final {
  linprog::Problem lp;               // c must be [n]; bounds [n]; rows as in linprog
  Series integer = pluss::series::undefined(); // [n] nonzero marks an integer variable
};

struct Result final {
  Series x = pluss::series::undefined();         // [n]
  Series objective = pluss::series::undefined(); // scalar (inf if no incumbent)
  double best_bound = -std::numeric_limits<double>::infinity();
  Status status = Status::Infeasible;
  int nodes = 0;
  int lp_rounds = 0;
};

namespace detail {
struct Node final {
  std::vector<double> lower, upper;
  double bound;
  int depth;
  std::vector<std::int64_t> basis; // parent's optimal simplex basis (warm start; empty for interior-point / root)
  std::vector<char> at_upper;
};
struct Worse final {
  bool operator()(const Node &a, const Node &b) const { return a.bound > b.bound || (a.bound == b.bound && a.depth < b.depth); }
};
} // namespace detail

inline Result solve(const Problem &p, const Options &opt = {}) {
  const Series &c0 = p.lp.c;
  if (!c0.defined() || c0.dim() != 1)
    throw std::invalid_argument("milp: c must be a 1-D Series");
  const std::int64_t n = c0.size(0);
  const auto dt = c0.scalar_type();
  const double inf = std::numeric_limits<double>::infinity();
  if (!p.integer.defined() || p.integer.numel() != n)
    throw std::invalid_argument("milp: `integer` must have one entry per variable");
  const std::vector<double> is_int = p.integer.to(dt).to_vec<double>();
  const std::vector<double> c_host = c0.to_vec<double>();
  std::vector<double> lo(static_cast<std::size_t>(n), -inf), hi(static_cast<std::size_t>(n), inf);
  if (p.lp.lower)
    lo = p.lp.lower->to(dt).to_vec<double>();
  if (p.lp.upper)
    hi = p.lp.upper->to(dt).to_vec<double>();
  for (std::int64_t j = 0; j < n; ++j) { // sentinel infinities (<= -1e20, >= 1e20) are real infinities, as in linprog.h
    if (lo[static_cast<std::size_t>(j)] <= -1e20) lo[static_cast<std::size_t>(j)] = -inf;
    if (hi[static_cast<std::size_t>(j)] >= 1e20) hi[static_cast<std::size_t>(j)] = inf;
    if (is_int[static_cast<std::size_t>(j)] != 0.0) {
      if (!std::isfinite(lo[static_cast<std::size_t>(j)]) || !std::isfinite(hi[static_cast<std::size_t>(j)]))
        throw std::invalid_argument("milp: integer variables must have finite lower and upper bounds");
      lo[static_cast<std::size_t>(j)] = std::ceil(lo[static_cast<std::size_t>(j)] - 1e-9);
      hi[static_cast<std::size_t>(j)] = std::floor(hi[static_cast<std::size_t>(j)] + 1e-9);
    }
  }

  // host copies of the rows for the (batched) rounding-feasibility check
  const linprog::Options lpo = opt.lp;
  const std::int64_t me = p.lp.a_eq ? p.lp.a_eq->size(0) : 0, mu = p.lp.a_ub ? p.lp.a_ub->size(0) : 0;
  const Series intmask = p.integer.to(dt).ne(0.0).to(dt); // [n]
  const Config cfg = Config{}.with_dtype(dt).with_platform(c0.platform());

  Result res;
  res.objective = pluss::series::zeros({}, cfg).add(inf);
  res.x = pluss::series::zeros({n}, cfg);
  double incumbent = inf;
  std::vector<double> best_x;
  std::priority_queue<detail::Node, std::vector<detail::Node>, detail::Worse> open;
  open.push({lo, hi, -inf, 0, {}, {}});
  bool first_round = true;

  auto prune_threshold = [&]() { return incumbent - std::max(opt.gap_tolerance, opt.gap_tolerance * std::abs(incumbent)) ; };

  while (!open.empty() && res.nodes < opt.max_nodes) {
    // ---- pop a batch of nodes (best bound first) -------------------------------------------------------
    std::vector<detail::Node> batch;
    while (!open.empty() && static_cast<int>(batch.size()) < opt.batch) {
      detail::Node nd = open.top();
      open.pop();
      if (incumbent < inf && nd.bound >= prune_threshold())
        continue;
      batch.push_back(std::move(nd));
    }
    if (batch.empty())
      break;
    const std::int64_t B = static_cast<std::int64_t>(batch.size());
    std::vector<double> lb_all, ub_all;
    for (const auto &nd : batch) {
      lb_all.insert(lb_all.end(), nd.lower.begin(), nd.lower.end());
      ub_all.insert(ub_all.end(), nd.upper.begin(), nd.upper.end());
    }
    const bool simplex = lpo.method == linprog::Method::Simplex;
    std::vector<double> status, obj, xs;
    std::vector<std::vector<std::int64_t>> node_basis(static_cast<std::size_t>(B));
    std::vector<std::vector<char>> node_upper(static_cast<std::size_t>(B));
    Series x_lanes = pluss::series::undefined();
    Series q_lower = pluss::series::undefined(), q_upper = pluss::series::undefined();
    std::optional<Series> q_b_ub, q_b_eq;
    q_lower = pluss::make_series(lb_all, {B, n}).to(dt).to(cfg.platform);
    q_upper = pluss::make_series(ub_all, {B, n}).to(dt).to(cfg.platform);
    if (mu > 0)
      q_b_ub = p.lp.b_ub->to(dt).unsqueeze(0).expand({B, mu}).contiguous();
    if (me > 0)
      q_b_eq = p.lp.b_eq->to(dt).unsqueeze(0).expand({B, me}).contiguous();
    if (!simplex) { // one batched interior-point call for the whole round
      linprog::Problem q;
      q.c = c0.unsqueeze(0).expand({B, n}).contiguous();
      q.a_eq = p.lp.a_eq;
      q.a_ub = p.lp.a_ub;
      q.b_eq = q_b_eq;
      q.b_ub = q_b_ub;
      q.lower = q_lower;
      q.upper = q_upper;
      const auto sol = linprog::solve(q, lpo);
      status = sol.status.to_vec<double>();
      obj = sol.objective.to_vec<double>();
      xs = sol.x.to_vec<double>();
      x_lanes = sol.x;
      // Soundness: a node LP the interior-point method could not finish must never be pruned as if it were infeasible.
      // Re-solve such nodes exactly (cold simplex) before using their result.
      bool patched = false;
      for (std::int64_t l = 0; l < B; ++l) {
        if (status[static_cast<std::size_t>(l)] != static_cast<double>(linprog::Status::IterationLimit))
          continue;
        linprog::Problem fq;
        fq.c = c0;
        fq.a_eq = p.lp.a_eq;
        fq.a_ub = p.lp.a_ub;
        fq.b_eq = p.lp.b_eq;
        fq.b_ub = p.lp.b_ub;
        fq.lower = q_lower.select(0, l);
        fq.upper = q_upper.select(0, l);
        linprog::Options exact = lpo;
        exact.method = linprog::Method::Simplex;
        const auto fix = linprog::solve(fq, exact);
        status[static_cast<std::size_t>(l)] = fix.status.item<double>();
        obj[static_cast<std::size_t>(l)] = fix.objective.item<double>();
        const auto xl = fix.x.to_vec<double>();
        std::copy(xl.begin(), xl.end(), xs.begin() + l * n);
        patched = true;
      }
      if (patched)
        x_lanes = pluss::make_series(xs, {B, n}).to(dt).to(cfg.platform);
    } else { // exact vertices: each node is warm-started from its parent's basis (a branching bound change is a dual-simplex repair)
      status.assign(static_cast<std::size_t>(B), 3.0);
      obj.assign(static_cast<std::size_t>(B), inf);
      xs.assign(static_cast<std::size_t>(B * n), 0.0);
      for (std::int64_t l = 0; l < B; ++l) {
        linprog::Problem q;
        q.c = c0;
        q.a_eq = p.lp.a_eq;
        q.a_ub = p.lp.a_ub;
        q.b_eq = p.lp.b_eq;
        q.b_ub = p.lp.b_ub;
        q.lower = q_lower.select(0, l);
        q.upper = q_upper.select(0, l);
        const auto &node = batch[static_cast<std::size_t>(l)];
        if (!node.basis.empty()) {
          q.warm_basis = node.basis;
          q.warm_at_upper = node.at_upper;
        }
        const auto sol = linprog::solve(q, lpo);
        status[static_cast<std::size_t>(l)] = sol.status.item<double>();
        obj[static_cast<std::size_t>(l)] = sol.objective.item<double>();
        const auto xl = sol.x.to_vec<double>();
        std::copy(xl.begin(), xl.end(), xs.begin() + l * n);
        if (!sol.basis.empty()) {
          node_basis[static_cast<std::size_t>(l)] = sol.basis[0];
          node_upper[static_cast<std::size_t>(l)] = sol.at_upper[0];
        }
      }
      x_lanes = pluss::make_series(xs, {B, n}).to(dt).to(cfg.platform);
    }
    ++res.lp_rounds;
    res.nodes += static_cast<int>(B);
    const linprog::Problem q_view = [&]() { linprog::Problem v; v.lower = q_lower; v.upper = q_upper; v.b_ub = q_b_ub; v.b_eq = q_b_eq; return v; }();

    if (first_round) {
      first_round = false;
      const int st0 = static_cast<int>(status[0]);
      if (st0 == static_cast<int>(linprog::Status::Unbounded)) {
        res.status = Status::Unbounded;
        return res;
      }
      if (st0 == static_cast<int>(linprog::Status::PrimalInfeasible)) {
        res.status = Status::Infeasible;
        return res;
      }
    }

    // ---- rounding heuristic for all lanes at once: round the integer coordinates, check the rows --------------
    {
      const Series xr = x_lanes.mul(intmask.neg().add(1.0)).add(x_lanes.round().mul(intmask));
      Series feas = pluss::series::ones({B}, cfg).to(ScalarType::Bool);
      feas = feas.logical_and(xr.ge(q_view.lower->sub(1e-7)).all(-1)).logical_and(xr.le(q_view.upper->add(1e-7)).all(-1));
      if (mu > 0) {
        const Series r = xr.matmul(p.lp.a_ub->to(dt).transpose(0, 1)).sub(*q_view.b_ub);
        feas = feas.logical_and(r.le(1e-6).all(-1));
      }
      if (me > 0) {
        const Series r = xr.matmul(p.lp.a_eq->to(dt).transpose(0, 1)).sub(*q_view.b_eq);
        feas = feas.logical_and(r.abs().le(1e-6).all(-1));
      }
      const Series val = xr.mul(c0).sum(-1, false);
      const auto fh = feas.to(dt).to_vec<double>(), vh = val.to_vec<double>(), xh = xr.to_vec<double>();
      for (std::int64_t l = 0; l < B; ++l)
        if (fh[static_cast<std::size_t>(l)] > 0.5 && vh[static_cast<std::size_t>(l)] < incumbent) {
          incumbent = vh[static_cast<std::size_t>(l)];
          best_x.assign(xh.begin() + l * n, xh.begin() + (l + 1) * n);
        }
    }

    // ---- branch -----------------------------------------------------------------------------------------
    for (std::int64_t l = 0; l < B; ++l) {
      const int st = static_cast<int>(status[static_cast<std::size_t>(l)]);
      if (!(st == static_cast<int>(linprog::Status::Optimal) || st == static_cast<int>(linprog::Status::NearOptimal)))
        continue; // infeasible (or failed) node: pruned
      const double node_obj = obj[static_cast<std::size_t>(l)];
      if (incumbent < inf && node_obj >= prune_threshold())
        continue;
      std::int64_t pick = -1;
      double worst = opt.integer_tolerance;
      for (std::int64_t j = 0; j < n; ++j) {
        if (is_int[static_cast<std::size_t>(j)] == 0.0)
          continue;
        const double v = xs[static_cast<std::size_t>(l * n + j)];
        const double frac = std::abs(v - std::round(v));
        if (frac > worst) {
          worst = frac;
          pick = j;
        }
      }
      const detail::Node &parent = batch[static_cast<std::size_t>(l)];
      if (pick < 0) { // integral relaxation: new incumbent candidate
        std::vector<double> xi(xs.begin() + l * n, xs.begin() + (l + 1) * n);
        double val = 0.0;
        for (std::int64_t j = 0; j < n; ++j) {
          if (is_int[static_cast<std::size_t>(j)] != 0.0)
            xi[static_cast<std::size_t>(j)] = std::round(xi[static_cast<std::size_t>(j)]);
          val += c_host[static_cast<std::size_t>(j)] * xi[static_cast<std::size_t>(j)];
        }
        if (val < incumbent) {
          incumbent = val;
          best_x = xi;
        }
        continue;
      }
      const double v = xs[static_cast<std::size_t>(l * n + pick)];
      detail::Node down = parent, up = parent;
      down.upper[static_cast<std::size_t>(pick)] = std::floor(v);
      up.lower[static_cast<std::size_t>(pick)] = std::ceil(v);
      down.bound = up.bound = node_obj;
      down.basis = up.basis = node_basis[static_cast<std::size_t>(l)];
      down.at_upper = up.at_upper = node_upper[static_cast<std::size_t>(l)];
      down.depth = up.depth = parent.depth + 1;
      if (down.upper[static_cast<std::size_t>(pick)] >= down.lower[static_cast<std::size_t>(pick)])
        open.push(std::move(down));
      if (up.lower[static_cast<std::size_t>(pick)] <= up.upper[static_cast<std::size_t>(pick)])
        open.push(std::move(up));
    }
  }

  res.best_bound = open.empty() ? incumbent : open.top().bound;
  if (best_x.empty()) {
    res.status = open.empty() ? Status::Infeasible : Status::NodeLimit;
    return res;
  }
  res.x = pluss::make_series(best_x, {n}).to(dt).to(cfg.platform);
  res.objective = pluss::series::zeros({}, cfg).add(incumbent);
  res.status = open.empty() || res.best_bound >= prune_threshold() ? Status::Optimal : Status::NodeLimit;
  return res;
}

} // namespace pluss::subsp::math::milp
