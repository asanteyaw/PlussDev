#pragma once

// Branch-and-cut MILP solver on the sparse dual simplex (simplex.h).
//
//   minimise c^T x + offset,  row_lo <= A x <= row_hi,  col_lo <= x <= col_hi,  x_j integer for integer[j]
//
// Search: best-bound node selection with plunging, every node LP warm-started from its parent's optimal basis by the dual simplex
// (bound changes keep dual feasibility) with an objective cutoff that stops hopeless LPs early. Branching on pseudocosts (strong
// branching initialises unreliable candidates). Incumbents are verified against the ORIGINAL data and repaired by an LP over the
// continuous variables if the LP solution is slightly off. Presolve (presolve.h) runs first and is undone on return.
//
//   Optimal    gap closed (relative / absolute tolerances)
//   Infeasible no feasible point exists
//   Unbounded  the LP relaxation is unbounded
//   NodeLimit / TimeLimit  stopped early: `x` holds the incumbent (if any), `best_bound` the proven bound

#include "pluss/subsp/math/lp/mip_cuts.h"
#include "pluss/subsp/math/lp/presolve.h"
#include "pluss/subsp/math/lp/simplex.h"
#include "pluss/subsp/math/lp/sparse.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <queue>
#include <unordered_map>
#include <vector>

namespace pluss::subsp::math::lp {

struct MipOptions final {
  double time_limit_seconds = 1e30;
  std::int64_t node_limit = 100'000'000;
  double rel_gap = 1e-6;
  double abs_gap = 1e-9;
  double int_tol = 1e-6;
  bool presolve = true;
  int verbose = 0;
  int strong_candidates = 8;      // candidates examined by strong branching
  int strong_iterations = 60;     // dual simplex iterations per strong-branching probe
  int reliability = 4;            // pseudocost observations before a variable is trusted
  int plunge_depth = 50;
  int cut_rounds = 25;            // root cutting-plane rounds (0 disables cuts)
  bool heuristics = true;         // diving, RENS / RINS sub-MIPs
  bool is_submip = false;         // internal: sub-MIPs of heuristics never start heuristics themselves
  double initial_cutoff = kInf;   // only solutions strictly better than this are of interest (sub-MIPs)
  const std::vector<double> *debug_solution = nullptr; // known feasible point (original space, presolve off): every cut must hold at it
  SimplexOptions simplex{};
};

struct MipResult final {
  SolveStatus status{SolveStatus::Infeasible};
  double objective{kInf};
  double best_bound{-kInf};
  std::vector<double> x;
  double gap{kInf};
  std::int64_t nodes{0};
  std::int64_t lp_iterations{0};
  double seconds{0.0};
};

inline MipResult solve_mip(const Model &model, const MipOptions &opt);

namespace mipdetail {

struct BoundChange final {
  std::int32_t var;
  double lo, hi;
};

struct Node final {
  std::shared_ptr<Node> parent;
  std::vector<BoundChange> changes;      // branching change + propagated tightenings of this node
  std::shared_ptr<std::vector<VarStatus>> basis; // parent's optimal basis (shared by both children)
  double bound{-kInf};                    // parent's LP objective
  double estimate{-kInf};
  std::int32_t depth{0};
};
using NodePtr = std::shared_ptr<Node>;
struct NodeOrder final {
  bool operator()(const NodePtr &a, const NodePtr &b) const { return a->bound > b->bound; }
};

} // namespace mipdetail

class MipSolver final {
public:
  MipSolver(const Model &model, const MipOptions &opt) : opt_(opt), orig_(model) { orig_.canonicalize_infinities(); }

  MipResult solve() {
    const auto t0 = std::chrono::steady_clock::now();
    start_ = t0;
    MipResult res;
    orig_.validate();
    // ---- presolve
    PresolveOptions po;
    po.enabled = opt_.presolve;
    PresolveResult pre;
    Model work = orig_;
    if (opt_.presolve) {
      pre = presolve(orig_, po);
      if (pre.status == SolveStatus::Infeasible) {
        res.status = SolveStatus::Infeasible;
        return finish(res);
      }
      if (pre.status == SolveStatus::Unbounded || pre.status == SolveStatus::InfeasibleOrUnbounded) {
        res.status = pre.status == SolveStatus::Unbounded ? SolveStatus::Unbounded : SolveStatus::InfeasibleOrUnbounded;
        // classify exactly with the unreduced LP relaxation
        Simplex lp(orig_, opt_.simplex);
        const auto s = lp.solve();
        res.status = s.status == SolveStatus::Infeasible ? SolveStatus::Infeasible : s.status == SolveStatus::Unbounded ? SolveStatus::Unbounded : res.status;
        return finish(res);
      }
      work = pre.reduced;
      has_pre_ = true;
      pre_ = std::move(pre);
    }
    model_ = work;
    n_ = model_.cols;
    if (model_.integer.empty())
      model_.integer.assign(static_cast<std::size_t>(n_), 0);
    const auto result_of_reduced = run_reduced();
    res = result_of_reduced;
    // ---- postsolve (a fully presolved model has no columns left: the point consists of the fixed values only)
    if (has_pre_ && (n_ == 0 || !res.x.empty()) && res.status == SolveStatus::Optimal)
      res.x = pre_.expand(res.x);
    else if (!res.x.empty() && has_pre_)
      res.x = pre_.expand(res.x);
    if (!res.x.empty() && !has_pre_)
      ;
    // objective in original units
    if (!res.x.empty()) {
      double obj = orig_.offset;
      for (std::int32_t j = 0; j < orig_.cols; ++j)
        obj += orig_.cost[static_cast<std::size_t>(j)] * res.x[static_cast<std::size_t>(j)];
      res.objective = obj;
    }
    if (has_pre_ && std::isfinite(res.best_bound)) {
      // reduced bounds already include the presolve offset (reduced.offset carries it)
    }
    return finish(res);
  }

private:
  using Node = mipdetail::Node;
  using NodePtr = mipdetail::NodePtr;
  using BoundChange = mipdetail::BoundChange;

  [[nodiscard]] double elapsed() const {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - start_).count();
  }
  MipResult finish(MipResult r) {
    r.seconds = elapsed();
    if (std::isfinite(r.objective) && std::isfinite(r.best_bound))
      r.gap = std::abs(r.objective - r.best_bound) / (1.0 + std::abs(r.objective));
    return r;
  }

  // ------------------------------------------------------------------ core search on the (presolved) model
  MipResult run_reduced() {
    MipResult res;
    const auto m = static_cast<std::size_t>(model_.rows), nn = static_cast<std::size_t>(n_);
    (void)m;
    is_int_.assign(nn, 0);
    for (std::size_t j = 0; j < nn; ++j)
      is_int_[j] = model_.integer[j];
    root_lo_ = model_.col_lo;
    root_hi_ = model_.col_hi;
    cur_lo_ = root_lo_;
    cur_hi_ = root_hi_;
    pc_down_sum_.assign(nn, 0.0);
    pc_up_sum_.assign(nn, 0.0);
    pc_down_n_.assign(nn, 0);
    pc_up_n_.assign(nn, 0);
    detect_objective_step();
    if (n_ == 0) { // everything fixed by presolve
      res.status = SolveStatus::Optimal;
      res.objective = model_.offset;
      res.best_bound = model_.offset;
      res.x.clear();
      return res;
    }
    SimplexOptions so = opt_.simplex;
    lp_ = std::make_unique<Simplex>(model_, so);
    lp_->set_time_limit(opt_.time_limit_seconds);
    // ---- root relaxation
    LpSolution root = lp_->solve();
    lp_iterations_ += root.iterations;
    if (root.status == SolveStatus::Infeasible) {
      if (opt_.verbose) std::fprintf(stderr, "root LP infeasible after %lld iters (rows %d cols %d)\n", (long long)root.iterations, (int)model_.rows, (int)model_.cols);
      res.status = SolveStatus::Infeasible;
      return res;
    }
    if (root.status == SolveStatus::Unbounded) {
      res.status = SolveStatus::Unbounded;
      return res;
    }
    if (root.status != SolveStatus::Optimal) {
      res.status = root.status == SolveStatus::TimeLimit ? SolveStatus::TimeLimit : SolveStatus::IterationLimit;
      return res;
    }
    if (!getenv("MIP_NOPROP")) {
      build_propagation();
      obj_idx_.clear();
      obj_val_.clear();
      for (std::int32_t j = 0; j < n_; ++j)
        if (model_.cost[static_cast<std::size_t>(j)] != 0.0) {
          obj_idx_.push_back(j);
          obj_val_.push_back(model_.cost[static_cast<std::size_t>(j)]);
        }
      if (model_.has_integers() && !probe_root()) {
        res.status = SolveStatus::Infeasible;
        return res;
      }
      if (model_.has_integers()) {
        const auto after = lp_->resolve();
        lp_iterations_ += after.iterations;
        if (after.status == SolveStatus::Optimal)
          root = after;
        else if (after.status == SolveStatus::Infeasible) {
          res.status = SolveStatus::Infeasible;
          return res;
        }
      }
    }
    if (opt_.cut_rounds > 0 && model_.has_integers())
      root = root_cut_loop(root);
    root_obj_ = root.objective;
    if (prop_ready_)
      build_propagation();
    if (std::isfinite(opt_.initial_cutoff))
      cutoff_ = opt_.initial_cutoff;
    if (opt_.heuristics && !opt_.is_submip && model_.has_integers()) {
      simple_rounding(root.x);
      run_dives(root, std::max<std::int64_t>(2000, 3 * root.iterations));
      rens(root);
    }
    auto root_basis = std::make_shared<std::vector<VarStatus>>(root.basis);
    std::priority_queue<NodePtr, std::vector<NodePtr>, mipdetail::NodeOrder> open;
    auto root_node = std::make_shared<Node>();
    root_node->bound = root_obj_;
    root_node->basis = root_basis;
    // process the root as a node: it is the first node popped
    open.push(root_node);
    first_root_solution_ = root;
    have_root_solution_ = true;
    std::int64_t nodes = 0;
    double global_bound = root_obj_;
    NodePtr plunge_next;
    int plunge_len = 0;
    while (!open.empty() || plunge_next) {
      if (elapsed() > opt_.time_limit_seconds) {
        res.status = SolveStatus::TimeLimit;
        break;
      }
      if (nodes >= opt_.node_limit) {
        res.status = SolveStatus::NodeLimit;
        break;
      }
      NodePtr node;
      if (plunge_next) {
        node = plunge_next;
        plunge_next.reset();
      } else {
        node = open.top();
        open.pop();
        plunge_len = 0;
      }
      if (node->bound >= cutoff_ - 1e-9 * (1.0 + std::abs(cutoff_)) && std::isfinite(cutoff_))
        continue; // pruned by bound before any work
      ++nodes;
      NodePtr child_down, child_up;
      const bool keep_going = process_node(node, nodes, child_down, child_up);
      if (!keep_going) {
        res.status = SolveStatus::TimeLimit;
        break;
      }
      // schedule children: plunge into the preferred one, queue the other
      if (child_down || child_up) {
        NodePtr first = child_down, second = child_up;
        if (child_down && child_up && prefer_up_) {
          first = child_up;
          second = child_down;
        }
        if (!first) {
          first = second;
          second = nullptr;
        }
        if (second)
          open.push(second);
        if (plunge_len < opt_.plunge_depth) {
          plunge_next = first;
          ++plunge_len;
        } else {
          open.push(first);
        }
      }
      // global lower bound
      double lb = open.empty() ? kInf : open.top()->bound;
      if (plunge_next)
        lb = std::min(lb, plunge_next->bound);
      global_bound = std::min(lb, std::isfinite(cutoff_) ? incumbent_obj_ : kInf);
      if (std::isfinite(incumbent_obj_) && gap_closed(incumbent_obj_, std::min(lb, incumbent_obj_))) {
        open = {};
        plunge_next.reset();
        break;
      }
      if (opt_.verbose && (nodes % 100 == 0 || nodes < 20))
        std::fprintf(stderr, "node %lld open %zu bound %.8g incumbent %.8g depth %d lp_iters %lld t=%.1fs\n", static_cast<long long>(nodes), open.size(), lb, incumbent_obj_,
                     node->depth, static_cast<long long>(lp_iterations_), elapsed());
    }
    res.nodes = nodes;
    res.lp_iterations = lp_iterations_;
    const bool exhausted = open.empty() && !plunge_next;
    if (res.status == SolveStatus::Infeasible || res.status == SolveStatus::Optimal || res.status == SolveStatus::IterationLimit) {
      // loop ended naturally or by gap
    }
    if (!incumbent_.empty()) {
      res.x = incumbent_;
      res.objective = incumbent_obj_;
      double lb = open.empty() ? incumbent_obj_ : open.top()->bound;
      if (plunge_next)
        lb = std::min(lb, plunge_next->bound);
      res.best_bound = std::min(lb, incumbent_obj_);
      if (res.status != SolveStatus::TimeLimit && res.status != SolveStatus::NodeLimit)
        res.status = SolveStatus::Optimal;
      if (exhausted)
        res.best_bound = incumbent_obj_;
      if (dropped_nodes_ > 0) { // honest accounting: a dropped node leaves its bound unproven
        res.best_bound = std::min(res.best_bound, dropped_bound_);
        if (!gap_closed(incumbent_obj_, res.best_bound))
          res.status = SolveStatus::NodeLimit;
      }
    } else {
      if (res.status != SolveStatus::TimeLimit && res.status != SolveStatus::NodeLimit)
        res.status = exhausted ? SolveStatus::Infeasible : res.status;
      res.best_bound = std::min(global_bound, kInf);
    }
    return res;
  }

  [[nodiscard]] bool gap_closed(double ub, double lb) const {
    return ub - lb <= std::max(opt_.abs_gap, opt_.rel_gap * (1.0 + std::abs(ub)));
  }

  // all-integer objective with integral coefficients: pruning step (gcd) makes the cutoff stronger
  void detect_objective_step() {
    obj_step_ = 0.0;
    bool ok = true;
    double g = 0.0;
    for (std::int32_t j = 0; j < n_ && ok; ++j) {
      const double c = model_.cost[static_cast<std::size_t>(j)];
      if (c == 0.0)
        continue;
      if (!is_int_[static_cast<std::size_t>(j)] || std::abs(c - std::round(c)) > 1e-9)
        ok = false;
      else {
        long long v = std::llround(std::abs(c));
        long long a = static_cast<long long>(g), b = v;
        while (b) {
          const long long t = a % b;
          a = b;
          b = t;
        }
        g = static_cast<double>(a);
      }
    }
    if (ok && g >= 1.0)
      obj_step_ = g;
  }

  void update_cutoff() {
    if (!std::isfinite(incumbent_obj_)) {
      cutoff_ = kInf;
      return;
    }
    const double eps = std::max(opt_.abs_gap, 1e-9 * (1.0 + std::abs(incumbent_obj_)));
    if (obj_step_ > 0.0)
      cutoff_ = incumbent_obj_ - obj_step_ + std::max(1e-6, 1e-9 * (1.0 + std::abs(incumbent_obj_)));
    else
      cutoff_ = incumbent_obj_ - eps;
  }


  // ------------------------------------------------------------------ node bound propagation
  // Activity-based bound tightening over the rows (and the objective cutoff c.x <= cutoff) starting from the columns whose bounds
  // differ from the root; tightened bounds go into cur_lo_/cur_hi_ and the LP. Returns false when a row proves the node infeasible.
  void build_propagation() {
    prop_csr_ = model_.a.transposed();
    prop_ready_ = true;
  }

  bool propagate_node(bool apply_lp = true) {
    if (!prop_ready_ || touched_.empty())
      return true;
    const auto m = static_cast<std::size_t>(prop_csr_.cols == 0 ? 0 : model_.rows);
    std::vector<char> queued(m, 0);
    std::vector<std::int32_t> queue;
    for (const auto j : touched_)
      for (auto p = model_.a.start[static_cast<std::size_t>(j)]; p < model_.a.start[static_cast<std::size_t>(j) + 1]; ++p) {
        const auto i = static_cast<std::size_t>(model_.a.index[static_cast<std::size_t>(p)]);
        if (i < m && !queued[i]) {
          queued[i] = 1;
          queue.push_back(static_cast<std::int32_t>(i));
        }
      }
    std::int64_t work = 0;
    const std::int64_t work_limit = 20 * (model_.a.nnz() + 1) / 4 + 2000;
    std::vector<std::int32_t> changed_cols;
    const auto tighten = [&](std::size_t j, double lo, double hi) {
      bool ch = false;
      if (is_int_[j]) {
        lo = std::ceil(lo - 1e-6);
        hi = std::floor(hi + 1e-6);
      }
      const double range = std::isfinite(cur_hi_[j] - cur_lo_[j]) ? cur_hi_[j] - cur_lo_[j] : 1e30;
      const double need = is_int_[j] ? 0.5 : std::max(1e-3 * range, 1e-6);
      if ((lo > cur_lo_[j] + need || hi < cur_hi_[j] - need) && prop_undo_)
        prop_undo_->push_back({static_cast<std::int32_t>(j), cur_lo_[j], cur_hi_[j]});
      if (lo > cur_lo_[j] + need) {
        cur_lo_[j] = lo;
        ch = true;
      }
      if (hi < cur_hi_[j] - need) {
        cur_hi_[j] = hi;
        ch = true;
      }
      if (ch)
        changed_cols.push_back(static_cast<std::int32_t>(j));
      return ch;
    };
    const auto row_pass = [&](const std::int32_t *idx, const double *val, std::size_t len, double rlo, double rhi, bool &infeasible) {
      double amin = 0.0, amax = 0.0;
      int min_inf = 0, max_inf = 0;
      for (std::size_t q = 0; q < len; ++q) {
        const auto j = static_cast<std::size_t>(idx[q]);
        const double v = val[q];
        if (v > 0) {
          if (std::isinf(cur_lo_[j])) ++min_inf; else amin += v * cur_lo_[j];
          if (std::isinf(cur_hi_[j])) ++max_inf; else amax += v * cur_hi_[j];
        } else {
          if (std::isinf(cur_hi_[j])) ++min_inf; else amin += v * cur_hi_[j];
          if (std::isinf(cur_lo_[j])) ++max_inf; else amax += v * cur_lo_[j];
        }
      }
      const double scale = 1.0 + std::abs(amin) + std::abs(amax);
      if ((!std::isinf(rhi) && min_inf == 0 && amin > rhi + 1e-7 * scale) || (!std::isinf(rlo) && max_inf == 0 && amax < rlo - 1e-7 * scale)) {
        infeasible = true;
        return;
      }
      for (std::size_t q = 0; q < len; ++q) {
        const auto j = static_cast<std::size_t>(idx[q]);
        const double v = val[q];
        if (std::abs(v) < 1e-9)
          continue;
        double lo = cur_lo_[j], hi = cur_hi_[j];
        if (!std::isinf(rhi)) {
          const bool own_inf = v > 0 ? std::isinf(cur_lo_[j]) : std::isinf(cur_hi_[j]);
          if (min_inf - (own_inf ? 1 : 0) == 0) {
            const double own = own_inf ? 0.0 : v * (v > 0 ? cur_lo_[j] : cur_hi_[j]);
            const double rest = rhi - (amin - own) + 1e-9 * scale;
            if (v > 0) hi = std::min(hi, rest / v); else lo = std::max(lo, rest / v);
          }
        }
        if (!std::isinf(rlo)) {
          const bool own_inf = v > 0 ? std::isinf(cur_hi_[j]) : std::isinf(cur_lo_[j]);
          if (max_inf - (own_inf ? 1 : 0) == 0) {
            const double own = own_inf ? 0.0 : v * (v > 0 ? cur_hi_[j] : cur_lo_[j]);
            const double rest = rlo - (amax - own) - 1e-9 * scale;
            if (v > 0) lo = std::max(lo, rest / v); else hi = std::min(hi, rest / v);
          }
        }
        if (hi < lo - 1e-7 * (1.0 + std::abs(lo))) {
          infeasible = true;
          return;
        }
        if (tighten(j, lo, hi)) {
          touched_.push_back(static_cast<std::int32_t>(j));
          for (auto p = model_.a.start[j]; p < model_.a.start[j + 1]; ++p) {
            const auto i2 = static_cast<std::size_t>(model_.a.index[static_cast<std::size_t>(p)]);
            if (i2 < m && !queued[i2]) {
              queued[i2] = 1;
              queue.push_back(static_cast<std::int32_t>(i2));
            }
          }
        }
      }
    };
    std::size_t head = 0;
    bool infeasible = false;
    while (head < queue.size() && work < work_limit && !infeasible) {
      const auto i = static_cast<std::size_t>(queue[head++]);
      queued[i] = 0;
      const auto beg = prop_csr_.start[i], end = prop_csr_.start[i + 1];
      work += end - beg;
      row_pass(prop_csr_.index.data() + beg, prop_csr_.value.data() + beg, static_cast<std::size_t>(end - beg), model_.row_lo[i], model_.row_hi[i], infeasible);
      if (head > 4 * m + 64)
        break;
    }
    // objective cutoff row  c.x <= cutoff
    if (!infeasible && std::isfinite(cutoff_) && obj_idx_.size() > 0) {
      row_pass(obj_idx_.data(), obj_val_.data(), obj_idx_.size(), -kInf, cutoff_ - model_.offset, infeasible);
    }
    if (infeasible)
      return false;
    std::sort(touched_.begin(), touched_.end());
    touched_.erase(std::unique(touched_.begin(), touched_.end()), touched_.end());
    if (apply_lp)
      for (const auto j : changed_cols)
        lp_->set_col_bounds(j, cur_lo_[static_cast<std::size_t>(j)], cur_hi_[static_cast<std::size_t>(j)]);
    return true;
  }
  struct UndoEntry final {
    std::int32_t j;
    double lo, hi;
  };
  std::vector<UndoEntry> *prop_undo_{nullptr};

  // Root probing on binaries (Savelsbergh 1994): tentatively fix a binary to 0 and to 1 and propagate. An infeasible side fixes the
  // variable to the other value; bounds implied by BOTH sides are valid globally. Returns false when the problem is infeasible.
  bool probe_root() {
    if (!prop_ready_ || opt_.is_submip || getenv("MIP_NOPROBE"))
      return true;
    const double t_end = elapsed() + std::min(0.03 * opt_.time_limit_seconds, 3.0);
    std::vector<std::int32_t> cand;
    for (std::int32_t j = 0; j < n_; ++j)
      if (is_int_[static_cast<std::size_t>(j)] && cur_lo_[static_cast<std::size_t>(j)] == 0.0 && cur_hi_[static_cast<std::size_t>(j)] == 1.0)
        cand.push_back(j);
    std::sort(cand.begin(), cand.end(), [&](auto a, auto b) {
      return model_.a.start[static_cast<std::size_t>(a) + 1] - model_.a.start[static_cast<std::size_t>(a)] > model_.a.start[static_cast<std::size_t>(b) + 1] - model_.a.start[static_cast<std::size_t>(b)];
    });
    if (cand.size() > 4000)
      cand.resize(4000);
    struct Bd {
      std::int32_t k;
      double lo, hi;
    };
    const auto global_set = [&](std::size_t k, double lo, double hi) {
      if (opt_.debug_solution && !opt_.debug_solution->empty() && !has_pre_) {
        const double dv = (*opt_.debug_solution)[k];
        if (dv < lo - 1e-9 || dv > hi + 1e-9)
          std::fprintf(stderr, "PROBING cuts off the known solution: var %zu value %g new bounds [%g, %g]\n", k, dv, lo, hi);
      }
      root_lo_[k] = cur_lo_[k] = std::max(cur_lo_[k], lo);
      root_hi_[k] = cur_hi_[k] = std::min(cur_hi_[k], hi);
      lp_->set_col_bounds(static_cast<std::int32_t>(k), cur_lo_[k], cur_hi_[k]);
    };
    bool any = false;
    std::int64_t tightened = 0;
    for (const auto j : cand) {
      if (elapsed() > t_end)
        break;
      const auto sj = static_cast<std::size_t>(j);
      if (cur_lo_[sj] == cur_hi_[sj])
        continue;
      std::vector<Bd> res[2];
      bool feas[2];
      for (int v = 0; v < 2; ++v) {
        std::vector<UndoEntry> undo;
        prop_undo_ = &undo;
        undo.push_back({j, cur_lo_[sj], cur_hi_[sj]});
        cur_lo_[sj] = cur_hi_[sj] = static_cast<double>(v);
        const auto saved_touched = touched_;
        touched_.assign(1, j);
        feas[v] = propagate_node(false);
        prop_undo_ = nullptr;
        if (feas[v])
          for (const auto &u : undo)
            if (u.j != j)
              res[v].push_back({u.j, cur_lo_[static_cast<std::size_t>(u.j)], cur_hi_[static_cast<std::size_t>(u.j)]});
        for (std::size_t q = undo.size(); q-- > 0;) {
          cur_lo_[static_cast<std::size_t>(undo[q].j)] = undo[q].lo;
          cur_hi_[static_cast<std::size_t>(undo[q].j)] = undo[q].hi;
        }
        touched_ = saved_touched;
      }
      if (!feas[0] && !feas[1])
        return false;
      if (!feas[0] || !feas[1]) {
        const double val = feas[0] ? 0.0 : 1.0;
        global_set(sj, val, val);
        // consequences of the surviving side are implied globally
        for (const auto &b : res[feas[0] ? 0 : 1])
          global_set(static_cast<std::size_t>(b.k), b.lo, b.hi);
        ++tightened;
        any = true;
        continue;
      }
      // both feasible: union of the implied bounds
      std::vector<Bd> merged;
      for (const auto &a : res[0]) {
        double lo = a.lo, hi = a.hi;
        bool found = false;
        for (const auto &b : res[1])
          if (b.k == a.k) {
            lo = std::min(lo, b.lo);
            hi = std::max(hi, b.hi);
            found = true;
          }
        if (!found) { // the 1-side leaves the variable at its current bounds
          lo = std::min(lo, cur_lo_[static_cast<std::size_t>(a.k)]);
          hi = std::max(hi, cur_hi_[static_cast<std::size_t>(a.k)]);
        }
        merged.push_back({a.k, lo, hi});
      }
      for (const auto &b : res[1]) {
        bool in0 = false;
        for (const auto &a : res[0])
          in0 |= a.k == b.k;
        if (!in0)
          merged.push_back({b.k, std::min(b.lo, cur_lo_[static_cast<std::size_t>(b.k)]), std::max(b.hi, cur_hi_[static_cast<std::size_t>(b.k)])});
      }
      for (const auto &b : merged) {
        const auto k = static_cast<std::size_t>(b.k);
        if (b.lo > cur_lo_[k] + 1e-9 || b.hi < cur_hi_[k] - 1e-9) {
          global_set(k, b.lo, b.hi);
          ++tightened;
          any = true;
        }
      }
    }
    touched_.clear();
    if (any) {
      model_.col_lo = root_lo_; // global bounds: later LP rebuilds and the cut generators must see the same box as the LP
      model_.col_hi = root_hi_;
      if (opt_.verbose)
        std::fprintf(stderr, "  probing tightened %lld bounds\n", static_cast<long long>(tightened));
      const auto again = lp_->resolve();
      lp_iterations_ += again.iterations;
      if (again.status == SolveStatus::Infeasible)
        return false;
    }
    return true;
  }
  SparseMatrix prop_csr_;
  bool prop_ready_{false};
  std::vector<std::int32_t> obj_idx_;
  std::vector<double> obj_val_;

  // ------------------------------------------------------------------ bounds of a node
  void apply_node_bounds(const NodePtr &node) {
    for (const auto j : touched_) {
      cur_lo_[static_cast<std::size_t>(j)] = root_lo_[static_cast<std::size_t>(j)];
      cur_hi_[static_cast<std::size_t>(j)] = root_hi_[static_cast<std::size_t>(j)];
      lp_->set_col_bounds(j, root_lo_[static_cast<std::size_t>(j)], root_hi_[static_cast<std::size_t>(j)]);
    }
    touched_.clear();
    std::vector<const Node *> chain;
    for (const Node *p = node.get(); p; p = p->parent.get())
      chain.push_back(p);
    for (std::size_t k = chain.size(); k-- > 0;)
      for (const auto &c : chain[k]->changes) {
        const auto j = static_cast<std::size_t>(c.var);
        cur_lo_[j] = std::max(cur_lo_[j], c.lo);
        cur_hi_[j] = std::min(cur_hi_[j], c.hi);
        touched_.push_back(c.var);
      }
    std::sort(touched_.begin(), touched_.end());
    touched_.erase(std::unique(touched_.begin(), touched_.end()), touched_.end());
    for (const auto j : touched_)
      lp_->set_col_bounds(j, cur_lo_[static_cast<std::size_t>(j)], cur_hi_[static_cast<std::size_t>(j)]);
  }


  // variable upper bounds  x_j <= u y_k  from two-term rows  a x_j + b y_k <= 0  (a > 0 > b, y binary, x_j >= 0 continuous)
  void detect_vubs(const SparseMatrix &csr) {
    vub_var_.assign(static_cast<std::size_t>(n_), -1);
    vub_coef_.assign(static_cast<std::size_t>(n_), 0.0);
    for (std::int32_t i = 0; i < model_.rows; ++i) {
      const auto si = static_cast<std::size_t>(i);
      const auto beg = csr.start[si], end = csr.start[si + 1];
      if (end - beg != 2)
        continue;
      for (int side = 0; side < 2; ++side) {
        const double bound = side == 0 ? model_.row_hi[si] : model_.row_lo[si];
        if (std::isinf(bound) || std::abs(bound) > 1e-12)
          continue;
        const double sg = side == 0 ? 1.0 : -1.0;
        const auto j0 = csr.index[static_cast<std::size_t>(beg)], j1 = csr.index[static_cast<std::size_t>(beg) + 1];
        const double a0 = sg * csr.value[static_cast<std::size_t>(beg)], a1 = sg * csr.value[static_cast<std::size_t>(beg) + 1];
        auto try_pair = [&](std::int32_t jx, double ax, std::int32_t jy, double ay) {
          const auto sx = static_cast<std::size_t>(jx), sy = static_cast<std::size_t>(jy);
          if (is_int_[sx] || !is_int_[sy] || root_lo_[sx] != 0.0 || root_lo_[sy] != 0.0 || root_hi_[sy] != 1.0 || ax <= 0 || ay >= 0)
            return;
          const double u = -ay / ax;
          if (vub_var_[sx] < 0 || u < vub_coef_[sx]) {
            vub_var_[sx] = jy;
            vub_coef_[sx] = u;
          }
        };
        try_pair(j0, a0, j1, a1);
        try_pair(j1, a1, j0, a0);
      }
    }
  }

  // ------------------------------------------------------------------ root cutting planes
  [[nodiscard]] static std::vector<char> integer_rows(const Model &m, const std::vector<char> &is_int, const SparseMatrix &csr) {
    std::vector<char> r(static_cast<std::size_t>(m.rows), 0);
    for (std::int32_t i = 0; i < m.rows; ++i) {
      bool ok = csr.start[static_cast<std::size_t>(i)] < csr.start[static_cast<std::size_t>(i) + 1];
      for (auto q = csr.start[static_cast<std::size_t>(i)]; q < csr.start[static_cast<std::size_t>(i) + 1] && ok; ++q) {
        ok = is_int[static_cast<std::size_t>(csr.index[static_cast<std::size_t>(q)])] != 0 &&
             std::abs(csr.value[static_cast<std::size_t>(q)] - std::round(csr.value[static_cast<std::size_t>(q)])) < 1e-9;
      }
      r[static_cast<std::size_t>(i)] = ok ? 1 : 0;
    }
    return r;
  }

  // append the cuts as rows (new logicals basic) and rebuild the LP warm from the previous basis
  void add_cuts(const std::vector<Cut> &cuts, std::vector<VarStatus> basis) {
    std::vector<std::int32_t> tr, tc;
    std::vector<double> tv;
    for (std::int32_t j = 0; j < n_; ++j)
      for (std::int64_t p = model_.a.start[static_cast<std::size_t>(j)]; p < model_.a.start[static_cast<std::size_t>(j) + 1]; ++p) {
        tr.push_back(model_.a.index[static_cast<std::size_t>(p)]);
        tc.push_back(j);
        tv.push_back(model_.a.value[static_cast<std::size_t>(p)]);
      }
    std::int32_t row = model_.rows;
    for (const auto &c : cuts) {
      for (std::size_t k = 0; k < c.index.size(); ++k) {
        tr.push_back(row);
        tc.push_back(c.index[k]);
        tv.push_back(c.value[k]);
      }
      model_.row_lo.push_back(c.lo);
      model_.row_hi.push_back(c.hi);
      if (!model_.row_names.empty())
        model_.row_names.push_back("cut");
      basis.push_back(VarStatus::Basic);
      ++row;
    }
    model_.rows = row;
    model_.a = SparseMatrix::from_triplets(model_.rows, n_, tr, tc, tv);
    lp_ = std::make_unique<Simplex>(model_, opt_.simplex);
    lp_->set_time_limit(opt_.time_limit_seconds);
    lp_->set_basis(basis);
    ++cut_row_rebuilds_;
  }

  LpSolution root_cut_loop(LpSolution root) {
    const std::int32_t m0 = model_.rows;
    int stall = 0;
    double last_obj = root.objective;
    std::int64_t total_cuts = 0;
    for (int round = 0; round < opt_.cut_rounds; ++round) {
      if (elapsed() > 0.3 * opt_.time_limit_seconds)
        break;
      // integral root: nothing to separate
      bool frac = false;
      for (std::int32_t j = 0; j < n_ && !frac; ++j)
        frac = is_int_[static_cast<std::size_t>(j)] && std::abs(root.x[static_cast<std::size_t>(j)] - std::round(root.x[static_cast<std::size_t>(j)])) > opt_.int_tol;
      if (!frac)
        break;
      const SparseMatrix csr = model_.a.transposed();
      const auto row_int = integer_rows(model_, is_int_, csr);
      detect_vubs(csr);
      CutContext ctx{model_, csr, is_int_, root_lo_, root_hi_, root.x, row_int, getenv("MIR_NOVUB") ? nullptr : &vub_var_, &vub_coef_, opt_.debug_solution};
      std::vector<Cut> pool;
      gomory_cuts(ctx, *lp_, pool, 200);
      mir_cuts(ctx, pool, 400);
      cover_cuts(ctx, pool, 600);
      if (!getenv("MIP_NOCLIQUE"))
        clique_cuts(ctx, pool, 400);
      if (opt_.debug_solution != nullptr && !opt_.debug_solution->empty()) {
        for (const auto &c : pool) {
          double act = 0.0;
          for (std::size_t k = 0; k < c.index.size(); ++k)
            act += c.value[k] * (*opt_.debug_solution)[static_cast<std::size_t>(c.index[k])];
          const double tol = 1e-6 * (1.0 + std::abs(act));
          if (act < c.lo - tol || act > c.hi + tol)
            std::fprintf(stderr, "INVALID CUT kind %d: activity at the known solution %.10g outside [%.10g, %.10g]\n", c.kind, act, c.lo, c.hi);
        }
      }
      const std::size_t keep = static_cast<std::size_t>(std::max<std::int32_t>(50, std::min<std::int32_t>(500, 4 * static_cast<std::int32_t>(n_) / 10)));
      auto cuts = select_cuts(std::move(pool), n_, keep);
      if (cuts.empty())
        break;
      total_cuts += static_cast<std::int64_t>(cuts.size());
      add_cuts(cuts, root.basis);
      auto next = lp_->resolve();
      lp_iterations_ += next.iterations;
      if (next.status != SolveStatus::Optimal) {
        if (next.status == SolveStatus::Infeasible) {
          root.status = SolveStatus::Infeasible;
          return root;
        }
        break; // numerical trouble: keep the last good LP (rebuilt below)
      }
      root = next;
      if (opt_.verbose)
        std::fprintf(stderr, "  cut round %d: %zu cuts, root bound %.10g\n", round, cuts.size(), root.objective);
      if (root.objective - last_obj < 1e-6 * (1.0 + std::abs(last_obj)) && ++stall >= 3)
        break;
      if (root.objective - last_obj >= 1e-6 * (1.0 + std::abs(last_obj)))
        stall = 0;
      last_obj = root.objective;
    }
    // purge cuts whose logical is basic (slack): rebuild without them
    if (model_.rows > m0) {
      std::vector<char> keep_row(static_cast<std::size_t>(model_.rows), 1);
      std::int32_t dropped = 0;
      for (std::int32_t i = m0; i < model_.rows; ++i)
        if (root.basis[static_cast<std::size_t>(n_ + i)] == VarStatus::Basic) {
          keep_row[static_cast<std::size_t>(i)] = 0;
          ++dropped;
        }
      if (dropped > 0) {
        std::vector<std::int32_t> new_idx(static_cast<std::size_t>(model_.rows), -1);
        std::int32_t cnt = 0;
        for (std::int32_t i = 0; i < model_.rows; ++i)
          if (keep_row[static_cast<std::size_t>(i)])
            new_idx[static_cast<std::size_t>(i)] = cnt++;
        std::vector<std::int32_t> tr, tc;
        std::vector<double> tv;
        for (std::int32_t j = 0; j < n_; ++j)
          for (std::int64_t p = model_.a.start[static_cast<std::size_t>(j)]; p < model_.a.start[static_cast<std::size_t>(j) + 1]; ++p) {
            const auto i = static_cast<std::size_t>(model_.a.index[static_cast<std::size_t>(p)]);
            if (keep_row[i]) {
              tr.push_back(new_idx[i]);
              tc.push_back(j);
              tv.push_back(model_.a.value[static_cast<std::size_t>(p)]);
            }
          }
        std::vector<double> rl, rh;
        std::vector<VarStatus> nb(root.basis.begin(), root.basis.begin() + n_);
        for (std::int32_t i = 0; i < model_.rows; ++i)
          if (keep_row[static_cast<std::size_t>(i)]) {
            rl.push_back(model_.row_lo[static_cast<std::size_t>(i)]);
            rh.push_back(model_.row_hi[static_cast<std::size_t>(i)]);
            nb.push_back(root.basis[static_cast<std::size_t>(n_ + i)]);
          }
        model_.rows = cnt;
        model_.row_lo = rl;
        model_.row_hi = rh;
        model_.a = SparseMatrix::from_triplets(cnt, n_, tr, tc, tv);
        model_.row_names.clear();
        lp_ = std::make_unique<Simplex>(model_, opt_.simplex);
        lp_->set_time_limit(opt_.time_limit_seconds);
        lp_->set_basis(nb);
        auto again = lp_->resolve();
        lp_iterations_ += again.iterations;
        if (again.status == SolveStatus::Optimal)
          root = again;
      }
    }
    cuts_added_ = total_cuts;
    return root;
  }

  // ------------------------------------------------------------------ one node: solve, bound, branch
  // Returns false only on a hard stop (time limit).
  bool process_node(const NodePtr &node, std::int64_t node_index, NodePtr &down, NodePtr &up) {
    apply_node_bounds(node);
    for (const auto j : touched_)
      if (cur_lo_[static_cast<std::size_t>(j)] > cur_hi_[static_cast<std::size_t>(j)] + 1e-9)
        return true; // contradictory bounds: infeasible node
    if (!opt_.is_submip || true) {
      if (!propagate_node())
        return true; // propagation proved the node infeasible
    }
    LpSolution sol;
    if (node->parent == nullptr && have_root_solution_) {
      sol = first_root_solution_; // root already solved
      have_root_solution_ = false;
    } else {
      if (node->basis && node->parent)
        lp_->set_basis(*node->basis);
      lp_->set_cutoff(cutoff_);
      lp_->set_time_limit(std::max(1e-3, opt_.time_limit_seconds - elapsed()));
      sol = lp_->resolve();
      lp_iterations_ += sol.iterations;
      if (sol.status == SolveStatus::TimeLimit)
        return false;
    }
    if (opt_.verbose > 1)
      std::fprintf(stderr, "  node %lld depth %d: LP status %d obj %.10g (cutoff %.10g, iters %lld)\n", static_cast<long long>(node_index), node->depth, static_cast<int>(sol.status), sol.objective, cutoff_, static_cast<long long>(sol.iterations));
    if (sol.status == SolveStatus::Infeasible || sol.status == SolveStatus::ObjectiveLimit)
      return true;
    if (sol.status != SolveStatus::Optimal) {
      // The node LP ended in an unexpected state (iteration/time limit, numerical failure). Dropping it silently would make every
      // optimality claim unsound, so it is recorded and its bound caps the proven bound.
      ++dropped_nodes_;
      dropped_bound_ = std::min(dropped_bound_, node->bound);
      if (opt_.verbose)
        std::fprintf(stderr, "  node %lld dropped: LP status %d\n", static_cast<long long>(node_index), static_cast<int>(sol.status));
      return true;
    }
    const double z = sol.objective;
    update_pseudocosts(node, z);
    if (z >= cutoff_ - 1e-9 * (1.0 + std::abs(cutoff_)) && std::isfinite(cutoff_))
      return true;
    // ---- fractional integer variables
    std::vector<std::int32_t> frac;
    for (std::int32_t j = 0; j < n_; ++j) {
      if (!is_int_[static_cast<std::size_t>(j)])
        continue;
      const double v = sol.x[static_cast<std::size_t>(j)];
      if (std::abs(v - std::round(v)) > opt_.int_tol)
        frac.push_back(j);
    }
    if (frac.empty()) {
      try_incumbent(sol.x);
      return true;
    }
    // ---- cheap rounding heuristic at shallow depth
    if (node->depth <= 10 || node_index % 20 == 0)
      simple_rounding(sol.x);
    if (opt_.heuristics && !opt_.is_submip) {
      if (node_index % 40 == 0)
        dive(incumbent_.empty() ? DiveRule::Fractional : DiveRule::Guided, sol, 2000);
      if (node_index % 250 == 0 && !incumbent_.empty())
        rins(sol);
    }
    // ---- branching variable
    std::int32_t bj = select_branching(node, sol, frac, z);
    if (bj < 0)
      return true; // strong branching proved the node infeasible / pruned
    const double v = sol.x[static_cast<std::size_t>(bj)];
    auto basis = std::make_shared<std::vector<VarStatus>>(sol.basis);
    std::vector<BoundChange> fixings;
    reduced_cost_fixing(sol, z, fixings);
    auto mk = [&](double lo, double hi) {
      auto c = std::make_shared<Node>();
      c->parent = node;
      c->changes.push_back({bj, lo, hi});
      for (const auto &f : fixings)
        if (f.var != bj)
          c->changes.push_back(f);
      c->basis = basis;
      c->bound = z;
      c->depth = node->depth + 1;
      return c;
    };
    down = mk(-kInf, std::floor(v));
    up = mk(std::ceil(v), kInf);
    last_branch_var_ = bj;
    last_branch_value_ = v;
    last_parent_obj_ = z;
    // direction preference: toward the child expected to be cheaper (pseudocost) -- default up for binaries
    const double fdown = v - std::floor(v), fup = std::ceil(v) - v;
    const double pd = pseudocost(bj, false) * fdown, pu = pseudocost(bj, true) * fup;
    prefer_up_ = pu < pd || (pu == pd && fup < fdown);
    down->estimate = z + pd;
    up->estimate = z + pu;
    node_branch_var_[down.get()] = {bj, fdown, false};
    node_branch_var_[up.get()] = {bj, fup, true};
    return true;
  }

  // ------------------------------------------------------------------ pseudocosts and branching
  struct BranchInfo final {
    std::int32_t var;
    double dist;
    bool up;
  };
  std::unordered_map<const Node *, BranchInfo> node_branch_var_;

  void update_pseudocosts(const NodePtr &node, double z) {
    auto it = node_branch_var_.find(node.get());
    if (it == node_branch_var_.end())
      return;
    const auto info = it->second;
    node_branch_var_.erase(it);
    if (info.dist <= 1e-9 || !std::isfinite(node->bound))
      return;
    const double gain = std::max(0.0, z - node->bound) / info.dist;
    const auto j = static_cast<std::size_t>(info.var);
    if (info.up) {
      pc_up_sum_[j] += gain;
      ++pc_up_n_[j];
    } else {
      pc_down_sum_[j] += gain;
      ++pc_down_n_[j];
    }
  }
  [[nodiscard]] double pseudocost(std::int32_t j, bool up) const {
    const auto sj = static_cast<std::size_t>(j);
    const double sum = up ? pc_up_sum_[sj] : pc_down_sum_[sj];
    const auto cnt = up ? pc_up_n_[sj] : pc_down_n_[sj];
    if (cnt > 0)
      return sum / static_cast<double>(cnt);
    // average over all observed pseudocosts of that direction
    double s = 0.0;
    std::int64_t c = 0;
    for (std::int32_t k = 0; k < n_; ++k) {
      const auto sk = static_cast<std::size_t>(k);
      s += up ? pc_up_sum_[sk] : pc_down_sum_[sk];
      c += up ? pc_up_n_[sk] : pc_down_n_[sk];
    }
    return c > 0 ? s / static_cast<double>(c) : 1.0;
  }

  // Reliability branching: candidates are ranked by pseudocost score; unreliable ones (fewer than `reliability` observations in a
  // direction) are probed by strong branching (a limited dual simplex on each child, warm from the current basis).
  // Returns -1 when both children of some probed candidate are infeasible / cut off (the node is pruned).
  std::int32_t select_branching(const NodePtr &node, const LpSolution &sol, const std::vector<std::int32_t> &frac, double z) {
    (void)node;
    struct Cand {
      std::int32_t j;
      double score;
    };
    std::vector<Cand> cands;
    cands.reserve(frac.size());
    for (const auto j : frac) {
      const double v = sol.x[static_cast<std::size_t>(j)];
      const double fd = v - std::floor(v), fu = std::ceil(v) - v;
      const double sd = std::max(pseudocost(j, false) * fd, 1e-6), su = std::max(pseudocost(j, true) * fu, 1e-6);
      cands.push_back({j, sd * su});
    }
    std::sort(cands.begin(), cands.end(), [](const Cand &a, const Cand &b) { return a.score > b.score; });
    const bool allow_strong = opt_.strong_candidates > 0 && (n_ + model_.rows) < 60000 && lp_->iterations() >= 0;
    std::int32_t best = cands.front().j;
    double best_score = -1.0;
    int probed = 0;
    int stall = 0;
    for (std::size_t k = 0; k < cands.size() && k < 400; ++k) {
      const auto j = cands[k].j;
      const auto sj = static_cast<std::size_t>(j);
      const double v = sol.x[sj];
      const double fd = v - std::floor(v), fu = std::ceil(v) - v;
      double gd, gu;
      const bool reliable = std::min(pc_down_n_[sj], pc_up_n_[sj]) >= opt_.reliability;
      if (!reliable && allow_strong && probed < opt_.strong_candidates) {
        ++probed;
        double od = 0.0, ou = 0.0;
        probe(j, v, z, od, ou);
        if (od >= kInf && ou >= kInf)
          return -1; // both children infeasible / cut off
        gd = od >= kInf ? 1e6 : std::max(0.0, od - z) / std::max(fd, 1e-9);
        gu = ou >= kInf ? 1e6 : std::max(0.0, ou - z) / std::max(fu, 1e-9);
        if (od < kInf) {
          pc_down_sum_[sj] += gd;
          ++pc_down_n_[sj];
        }
        if (ou < kInf) {
          pc_up_sum_[sj] += gu;
          ++pc_up_n_[sj];
        }
      } else {
        gd = pseudocost(j, false);
        gu = pseudocost(j, true);
      }
      const double score = std::max(gd * fd, 1e-6) * std::max(gu * fu, 1e-6);
      if (score > best_score * (1.0 + 1e-9)) {
        best_score = score;
        best = j;
        stall = 0;
      } else if (++stall >= 8 && probed >= opt_.strong_candidates)
        break;
    }
    return best;
  }

  // Strong-branching probe: objective lower bounds of both children (kInf = infeasible or cut off).
  void probe(std::int32_t j, double v, double z, double &obj_down, double &obj_up) {
    const auto sj = static_cast<std::size_t>(j);
    (void)z;
    auto snap = lp_->save();
    const double lo = cur_lo_[sj], hi = cur_hi_[sj];
    for (int side = 0; side < 2; ++side) {
      if (side == 0)
        lp_->set_col_bounds(j, lo, std::floor(v));
      else
        lp_->set_col_bounds(j, std::ceil(v), hi);
      lp_->set_iteration_limit(opt_.strong_iterations);
      lp_->set_cutoff(cutoff_);
      auto r = lp_->resolve();
      lp_iterations_ += r.iterations;
      double val;
      if (r.status == SolveStatus::Infeasible || r.status == SolveStatus::ObjectiveLimit)
        val = kInf;
      else if (r.status == SolveStatus::Optimal)
        val = r.objective;
      else
        val = lp_->current_objective(); // iteration-limited: the dual objective is still a valid bound
      (side == 0 ? obj_down : obj_up) = val;
      lp_->restore(snap);
      lp_->set_col_bounds(j, lo, hi);
    }
    lp_->set_iteration_limit(0);
  }

  // Reduced-cost fixing against the incumbent: tightenings valid in the whole subtree (returned as bound changes).
  void reduced_cost_fixing(const LpSolution &sol, double z, std::vector<BoundChange> &out) const {
    if (!std::isfinite(cutoff_) || sol.reduced.empty())
      return;
    const double gap = cutoff_ - z;
    if (gap < 0)
      return;
    for (std::int32_t j = 0; j < n_; ++j) {
      const auto sj = static_cast<std::size_t>(j);
      if (!is_int_[sj] || cur_lo_[sj] >= cur_hi_[sj])
        continue;
      const double d = sol.reduced[sj];
      const VarStatus st = sol.basis[sj];
      if (st == VarStatus::AtLower && d > 1e-6) {
        const double room = std::floor(gap / d + 1e-6);
        if (cur_lo_[sj] + room < cur_hi_[sj] - 0.5)
          out.push_back({j, -kInf, cur_lo_[sj] + room});
      } else if (st == VarStatus::AtUpper && d < -1e-6) {
        const double room = std::floor(gap / -d + 1e-6);
        if (cur_hi_[sj] - room > cur_lo_[sj] + 0.5)
          out.push_back({j, cur_hi_[sj] - room, kInf});
      }
    }
  }


  // ------------------------------------------------------------------ primal heuristics
  void compute_locks() {
    uplock_.assign(static_cast<std::size_t>(n_), 0);
    downlock_.assign(static_cast<std::size_t>(n_), 0);
    for (std::int32_t j = 0; j < n_; ++j)
      for (std::int64_t p = model_.a.start[static_cast<std::size_t>(j)]; p < model_.a.start[static_cast<std::size_t>(j) + 1]; ++p) {
        const auto i = static_cast<std::size_t>(model_.a.index[static_cast<std::size_t>(p)]);
        const double v = model_.a.value[static_cast<std::size_t>(p)];
        const bool has_lo = !std::isinf(model_.row_lo[i]), has_hi = !std::isinf(model_.row_hi[i]);
        if (v > 0) {
          if (has_hi) ++uplock_[static_cast<std::size_t>(j)];
          if (has_lo) ++downlock_[static_cast<std::size_t>(j)];
        } else {
          if (has_lo) ++uplock_[static_cast<std::size_t>(j)];
          if (has_hi) ++downlock_[static_cast<std::size_t>(j)];
        }
      }
  }

  enum class DiveRule { Fractional, Coefficient, Guided, Pseudocost };

  // LP diving: repeatedly fix a fractional integer variable (bound change) and re-solve with the dual simplex; one backtrack per level.
  bool dive(DiveRule rule, const LpSolution &start, std::int64_t iter_budget) {
    if (uplock_.empty())
      compute_locks();
    auto snap = lp_->save();
    const auto saved_lo = cur_lo_, saved_hi = cur_hi_; // MIP-side bounds are untouched; LP bounds restored through the snapshot
    std::vector<double> lo = cur_lo_, hi = cur_hi_;
    LpSolution cur = start;
    bool success = false;
    std::int64_t used = 0;
    for (int depth = 0; depth < 1000 && used < iter_budget; ++depth) {
      std::int32_t best = -1;
      double best_score = kInf;
      bool best_up = false;
      for (std::int32_t j = 0; j < n_; ++j) {
        const auto sj = static_cast<std::size_t>(j);
        if (!is_int_[sj] || lo[sj] >= hi[sj])
          continue;
        const double v = cur.x[sj];
        const double f = v - std::floor(v);
        if (f < opt_.int_tol || f > 1.0 - opt_.int_tol)
          continue;
        double score;
        bool up;
        switch (rule) {
        case DiveRule::Fractional:
          up = f > 0.5;
          score = std::min(f, 1.0 - f);
          break;
        case DiveRule::Coefficient: {
          const bool up_ok = uplock_[sj] <= downlock_[sj];
          up = up_ok;
          score = static_cast<double>(std::min(uplock_[sj], downlock_[sj])) + std::min(f, 1.0 - f) * 0.5;
          break;
        }
        case DiveRule::Guided:
          if (incumbent_.empty()) {
            up = f > 0.5;
            score = std::min(f, 1.0 - f);
          } else {
            up = incumbent_[sj] > v;
            score = std::abs(incumbent_[sj] - v);
          }
          break;
        default: { // pseudocost
          const double pd = pseudocost(j, false) * f, pu = pseudocost(j, true) * (1.0 - f);
          up = pu < pd;
          score = std::min(pd, pu) / (1e-6 + std::max(pd, pu));
        }
        }
        if (score < best_score) {
          best_score = score;
          best = j;
          best_up = up;
        }
      }
      if (best < 0) { // integral
        try_incumbent(cur.x);
        success = true;
        break;
      }
      const auto sb = static_cast<std::size_t>(best);
      bool solved = false;
      for (int attempt = 0; attempt < 2 && !solved; ++attempt) {
        const bool up = attempt == 0 ? best_up : !best_up;
        const double old_lo = lo[sb], old_hi = hi[sb];
        double nlo = old_lo, nhi = old_hi;
        if (up)
          nlo = std::ceil(cur.x[sb]);
        else
          nhi = std::floor(cur.x[sb]);
        if (nlo > nhi)
          continue;
        lp_->set_col_bounds(best, nlo, nhi);
        lp_->set_cutoff(cutoff_);
        lp_->set_iteration_limit(std::max<std::int64_t>(100, iter_budget - used));
        auto r = lp_->resolve();
        used += r.iterations;
        lp_iterations_ += r.iterations;
        if (r.status == SolveStatus::Optimal) {
          lo[sb] = nlo;
          hi[sb] = nhi;
          cur = r;
          solved = true;
          simple_rounding(cur.x);
        } else {
          lp_->set_col_bounds(best, old_lo, old_hi);
          if (r.status == SolveStatus::TimeLimit)
            attempt = 2;
        }
      }
      if (!solved)
        break;
    }
    lp_->restore(snap);
    lp_->set_iteration_limit(0);
    (void)saved_lo;
    (void)saved_hi;
    return success;
  }

  // Sub-MIP heuristic on a restricted model; any solution found is verified against the full model and accepted.
  void submip(const std::vector<double> &lo, const std::vector<double> &hi, double time_share, std::int64_t node_limit) {
    if (opt_.is_submip || !std::isfinite(time_share))
      return;
    Model sub = model_;
    int fixed = 0, ints = 0;
    for (std::int32_t j = 0; j < n_; ++j) {
      const auto sj = static_cast<std::size_t>(j);
      sub.col_lo[sj] = lo[sj];
      sub.col_hi[sj] = hi[sj];
      if (is_int_[sj]) {
        ++ints;
        fixed += lo[sj] >= hi[sj];
      }
    }
    if (ints == 0 || fixed < 0.3 * ints)
      return; // too little restriction to be a cheap sub-problem
    MipOptions so = opt_;
    so.is_submip = true;
    so.debug_solution = nullptr; // the restricted sub-problem legitimately excludes the known solution
    so.heuristics = false;
    so.verbose = 0;
    so.cut_rounds = 5;
    so.node_limit = node_limit;
    so.time_limit_seconds = std::max(0.5, std::min(time_share, opt_.time_limit_seconds - elapsed()));
    so.initial_cutoff = std::isfinite(cutoff_) ? cutoff_ : kInf;
    so.strong_candidates = 4;
    auto r = solve_mip(sub, so);
    if (!r.x.empty()) {
      try_incumbent(r.x);
    }
  }

  void rens(const LpSolution &root) {
    std::vector<double> lo = cur_lo_, hi = cur_hi_;
    for (std::int32_t j = 0; j < n_; ++j) {
      const auto sj = static_cast<std::size_t>(j);
      if (!is_int_[sj])
        continue;
      const double v = root.x[sj];
      if (std::abs(v - std::round(v)) <= opt_.int_tol)
        lo[sj] = hi[sj] = std::round(v);
      else {
        lo[sj] = std::max(lo[sj], std::floor(v));
        hi[sj] = std::min(hi[sj], std::ceil(v));
      }
    }
    submip(lo, hi, std::min(30.0, 0.15 * opt_.time_limit_seconds), 1000);
  }

  void rins(const LpSolution &node_sol) {
    if (incumbent_.empty())
      return;
    std::vector<double> lo = cur_lo_, hi = cur_hi_;
    for (std::int32_t j = 0; j < n_; ++j) {
      const auto sj = static_cast<std::size_t>(j);
      if (is_int_[sj] && std::abs(node_sol.x[sj] - incumbent_[sj]) < 0.5)
        lo[sj] = hi[sj] = incumbent_[sj];
    }
    submip(lo, hi, std::min(15.0, 0.1 * opt_.time_limit_seconds), 500);
  }

  void run_dives(const LpSolution &sol, std::int64_t budget) {
    for (const auto rule : {DiveRule::Fractional, DiveRule::Coefficient, DiveRule::Pseudocost, DiveRule::Guided}) {
      if (elapsed() > 0.5 * opt_.time_limit_seconds)
        return;
      if (rule == DiveRule::Guided && incumbent_.empty())
        continue;
      dive(rule, sol, budget);
    }
  }

  // ------------------------------------------------------------------ incumbents
  // Check a candidate against the working model; round integers exactly.
  bool feasible(const std::vector<double> &x, double tol = 1e-6) const {
    for (std::int32_t j = 0; j < n_; ++j) {
      const auto sj = static_cast<std::size_t>(j);
      if (x[sj] < root_lo_[sj] - tol * (1.0 + std::abs(root_lo_[sj])) || x[sj] > root_hi_[sj] + tol * (1.0 + std::abs(root_hi_[sj])))
        return false;
      if (is_int_[sj] && std::abs(x[sj] - std::round(x[sj])) > 1e-9)
        return false;
    }
    std::vector<double> act(static_cast<std::size_t>(model_.rows), 0.0);
    model_.a.multiply_add(1.0, x, act);
    for (std::int32_t i = 0; i < model_.rows; ++i) {
      const auto si = static_cast<std::size_t>(i);
      const double s = 1.0 + std::abs(act[si]);
      if (act[si] < model_.row_lo[si] - tol * s || act[si] > model_.row_hi[si] + tol * s)
        return false;
    }
    return true;
  }

  double objective_of(const std::vector<double> &x) const {
    double obj = model_.offset;
    for (std::int32_t j = 0; j < n_; ++j)
      obj += model_.cost[static_cast<std::size_t>(j)] * x[static_cast<std::size_t>(j)];
    return obj;
  }

  void accept(const std::vector<double> &x) {
    const double obj = objective_of(x);
    if (obj < incumbent_obj_) {
      incumbent_ = x;
      incumbent_obj_ = obj;
      update_cutoff();
      if (opt_.verbose)
        std::fprintf(stderr, "  new incumbent %.10g (t=%.1fs)\n", obj, elapsed());
    }
  }

  void try_incumbent(std::vector<double> x) {
    for (std::int32_t j = 0; j < n_; ++j)
      if (is_int_[static_cast<std::size_t>(j)])
        x[static_cast<std::size_t>(j)] = std::round(x[static_cast<std::size_t>(j)]);
    if (feasible(x)) {
      accept(x);
      return;
    }
    // repair: fix the integers, re-optimise the continuous part with a fresh LP
    Model fixed = model_;
    for (std::int32_t j = 0; j < n_; ++j)
      if (is_int_[static_cast<std::size_t>(j)]) {
        fixed.col_lo[static_cast<std::size_t>(j)] = fixed.col_hi[static_cast<std::size_t>(j)] = x[static_cast<std::size_t>(j)];
      }
    Simplex lp(fixed, opt_.simplex);
    const auto s = lp.solve();
    if (s.status == SolveStatus::Optimal && feasible(s.x, 5e-6))
      accept(s.x);
  }

  // round every fractional integer in the direction that cannot violate rows (lock-based), accept if still feasible
  void simple_rounding(const std::vector<double> &x0) {
    std::vector<double> x = x0;
    for (std::int32_t j = 0; j < n_; ++j) {
      const auto sj = static_cast<std::size_t>(j);
      if (!is_int_[sj])
        continue;
      if (std::abs(x[sj] - std::round(x[sj])) <= opt_.int_tol) {
        x[sj] = std::round(x[sj]);
        continue;
      }
      bool up_ok = true, down_ok = true;
      for (std::int64_t p = model_.a.start[sj]; p < model_.a.start[sj + 1]; ++p) {
        const auto i = static_cast<std::size_t>(model_.a.index[static_cast<std::size_t>(p)]);
        const double v = model_.a.value[static_cast<std::size_t>(p)];
        const bool has_lo = !std::isinf(model_.row_lo[i]), has_hi = !std::isinf(model_.row_hi[i]);
        if (v > 0) {
          if (has_hi) up_ok = false;
          if (has_lo) down_ok = false;
        } else {
          if (has_lo) up_ok = false;
          if (has_hi) down_ok = false;
        }
      }
      if (up_ok)
        x[sj] = std::ceil(x[sj]);
      else if (down_ok)
        x[sj] = std::floor(x[sj]);
      else
        return;
    }
    if (feasible(x))
      accept(x);
  }

  MipOptions opt_;
  Model orig_, model_;
  PresolveResult pre_;
  bool has_pre_{false};
  std::int32_t n_{0};
  std::chrono::steady_clock::time_point start_{std::chrono::steady_clock::now()};
  std::unique_ptr<Simplex> lp_;
  std::vector<char> is_int_;
  std::vector<double> root_lo_, root_hi_, cur_lo_, cur_hi_;
  std::vector<std::int32_t> touched_;
  std::vector<double> pc_down_sum_, pc_up_sum_;
  std::vector<std::int64_t> pc_down_n_, pc_up_n_;
  std::vector<double> incumbent_;
  double incumbent_obj_{kInf}, cutoff_{kInf}, obj_step_{0.0}, root_obj_{0.0};
  std::int64_t lp_iterations_{0};
  LpSolution first_root_solution_;
  bool have_root_solution_{false}, prefer_up_{false};
  std::int64_t cuts_added_{0}, cut_row_rebuilds_{0}, dropped_nodes_{0};
  double dropped_bound_{kInf};
  std::vector<std::int32_t> uplock_, downlock_, vub_var_;
  std::vector<double> vub_coef_;
  std::int32_t last_branch_var_{-1};
  double last_branch_value_{0.0}, last_parent_obj_{0.0};
};

inline MipResult solve_mip(const Model &model, const MipOptions &opt = {}) {
  MipSolver s(model, opt);
  return s.solve();
}

} // namespace pluss::subsp::math::lp
