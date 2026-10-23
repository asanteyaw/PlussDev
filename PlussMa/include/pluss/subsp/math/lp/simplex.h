#pragma once

// Sparse revised simplex (dual simplex core + primal simplex) for  min c^T x,  row_lo <= A x <= row_hi,  col_lo <= x <= col_hi.
//
// Variables 0..n-1 are structural, n..n+m-1 are LOGICALS l_i = a_i^T x (constraint  A x - l = 0, so the logical column is -e_i and
// the initial basis is the identity). All bounds live on the variables; a basis is a status per variable.
//
//   dual simplex  : dual steepest-edge pricing (Forrest & Goldfarb), bound-flipping ratio test with Harris tolerances (Maros;
//                   Koberstein 2005), cost perturbation + cost shifting against dual degeneracy, dual phase I by the
//                   artificial-bounds subproblem (Koberstein), warm start from any basis after bound changes.
//   primal simplex: Devex pricing, Harris two-pass ratio test, bound flips; used to remove cost perturbations, to certify
//                   unboundedness (ray) and as a fallback.
//   basis         : sparse LU (lu.h) with product-form updates, refactorised every `refactor_interval` iterations or when the
//                   eta file outgrows the factors; singular bases are repaired with logicals.
// Reference: Koberstein, "The dual simplex method, techniques for a fast and stable implementation" (PhD thesis, Paderborn 2005);
// Forrest & Goldfarb, Math. Programming 57 (1992); Maros, "Computational Techniques of the Simplex Method" (2003).

#include "pluss/subsp/math/lp/lu.h"
#include "pluss/subsp/math/lp/sparse.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cmath>
#include <cstdint>
#include <numeric>
#include <random>
#include <vector>

namespace pluss::subsp::math::lp {

enum class VarStatus : std::int8_t { Basic = 0, AtLower = 1, AtUpper = 2, Free = 3 };

struct SimplexOptions final {
  double primal_tol = 1e-7;
  double dual_tol = 1e-7;
  double pivot_tol = 1e-7;
  std::int64_t max_iterations = 5'000'000;
  double time_limit_seconds = 1e30;
  const std::atomic<bool> *stop = nullptr; // cooperative cancellation (another solver finished first): reported as TimeLimit
  int refactor_interval = 300;
  bool perturb = true;
  bool scale = true;
  bool verbose = false;
};

struct LpSolution final {
  SolveStatus status{SolveStatus::IterationLimit};
  double objective{0.0};
  std::vector<double> x;           // structural values
  std::vector<double> row_activity;
  std::vector<double> row_dual;    // y: d_j = c_j - y^T a_j
  std::vector<double> reduced;     // reduced costs of structurals
  std::vector<VarStatus> basis;    // size cols + rows
  std::vector<double> ray;         // primal unbounded ray (structurals) when Unbounded
  std::vector<double> farkas;      // row multipliers of an infeasibility proof when Infeasible
  std::int64_t iterations{0};
};

class Simplex final {
public:
  Simplex(const Model &model, SimplexOptions opt = {}) : default_budget_(opt.max_iterations), opt_(opt) { load(model); }

  // ---- model edits for warm-started re-solves (branch & bound, cut loops) ----
  void set_col_bounds(std::int32_t j, double lo, double hi) {
    lo_[static_cast<std::size_t>(j)] = lo * col_scale_inv_[static_cast<std::size_t>(j)];
    hi_[static_cast<std::size_t>(j)] = hi * col_scale_inv_[static_cast<std::size_t>(j)];
    if (std::isinf(lo))
      lo_[static_cast<std::size_t>(j)] = lo;
    if (std::isinf(hi))
      hi_[static_cast<std::size_t>(j)] = hi;
    orig_lo_[static_cast<std::size_t>(j)] = lo_[static_cast<std::size_t>(j)];
    orig_hi_[static_cast<std::size_t>(j)] = hi_[static_cast<std::size_t>(j)];
    primal_dirty_ = true;
  }
  void get_basis(std::vector<VarStatus> &out) const { out.assign(st_.begin(), st_.end()); }
  // Install a basis (size n+m, exactly m basics). Returns false (and keeps the slack basis) when invalid.
  bool set_basis(const std::vector<VarStatus> &b) {
    if (static_cast<std::int32_t>(b.size()) != n_ + m_)
      return false;
    std::int32_t nb = 0;
    for (const auto s : b)
      nb += s == VarStatus::Basic;
    if (nb != m_)
      return false;
    st_ = b;
    std::int32_t k = 0;
    std::fill(pos_.begin(), pos_.end(), -1);
    for (std::int32_t j = 0; j < n_ + m_; ++j)
      if (st_[static_cast<std::size_t>(j)] == VarStatus::Basic) {
        basic_[static_cast<std::size_t>(k)] = j;
        pos_[static_cast<std::size_t>(j)] = k++;
      }
    place_nonbasic();
    have_basis_ = true;
    need_refactor_ = true;
    return true;
  }
  // ---- re-solve support (branch & bound, strong branching, cut loops) ----
  struct Snapshot final {
    std::vector<VarStatus> st;
    std::vector<double> x, cost, lo, hi, dse, orig_cost, orig_lo, orig_hi;
    std::vector<std::int32_t> basic, pos;
    BasisFactor lu;
    bool shifted{false};
  };
  [[nodiscard]] Snapshot save() const { return {st_, x_, cost_, lo_, hi_, dse_, orig_cost_, orig_lo_, orig_hi_, basic_, pos_, lu_, shifted_}; }
  void restore(const Snapshot &s) {
    st_ = s.st;
    x_ = s.x;
    cost_ = s.cost;
    lo_ = s.lo;
    hi_ = s.hi;
    dse_ = s.dse;
    orig_cost_ = s.orig_cost;
    orig_lo_ = s.orig_lo;
    orig_hi_ = s.orig_hi;
    basic_ = s.basic;
    pos_ = s.pos;
    lu_ = s.lu;
    shifted_ = s.shifted;
    have_basis_ = true;
    need_refactor_ = false;
    primal_dirty_ = false;
    compute_dual();
  }
  void set_cost(std::int32_t j, double c) {
    orig_cost_[static_cast<std::size_t>(j)] = c * col_scale_[static_cast<std::size_t>(j)];
    cost_[static_cast<std::size_t>(j)] = orig_cost_[static_cast<std::size_t>(j)];
    costs_dirty_ = true;
  }
  // limit on iterations PER solve()/resolve() call (0 = default)
  void set_iteration_limit(std::int64_t limit) { per_solve_limit_ = limit; }
  void set_time_limit(double seconds) { opt_.time_limit_seconds = seconds; }
  // Objective values above `cutoff` (in model units, minimisation) stop the dual simplex with SolveStatus::ObjectiveLimit.
  void set_cutoff(double cutoff) { cutoff_scaled_ = cutoff - offset_; }
  [[nodiscard]] double column_value(std::int32_t j) const { return x_[static_cast<std::size_t>(j)] * col_scale_[static_cast<std::size_t>(j)]; }
  [[nodiscard]] double reduced_cost(std::int32_t j) const { return d_[static_cast<std::size_t>(j)] / col_scale_[static_cast<std::size_t>(j)]; }
  [[nodiscard]] VarStatus status(std::int32_t var) const { return st_[static_cast<std::size_t>(var)]; }
  [[nodiscard]] std::int64_t iterations() const { return iterations_; }
  [[nodiscard]] double current_objective() const {
    double obj = offset_;
    for (std::int32_t j = 0; j < n_; ++j)
      obj += orig_cost_[static_cast<std::size_t>(j)] * x_[static_cast<std::size_t>(j)];
    return obj;
  }
  // Tableau row of the basic variable at position `pos` over the NONBASIC variables, in UNSCALED structural / logical units:
  //   x_basic + sum_j row[j] * x_j = 0   (j over nonbasic variables, structurals then logicals). Used for Gomory cuts.
  void tableau_row(std::int32_t position, std::vector<double> &row_out) {
    std::vector<double> rho(static_cast<std::size_t>(m_), 0.0), row(static_cast<std::size_t>(n_ + m_), 0.0);
    std::vector<std::int32_t> touched;
    rho[static_cast<std::size_t>(position)] = 1.0;
    lu_.btran(rho);
    compute_pivot_row(rho, row, touched);
    row_out.assign(static_cast<std::size_t>(n_ + m_), 0.0);
    const auto bv = static_cast<std::size_t>(basic_[static_cast<std::size_t>(position)]);
    const double bscale = bv < static_cast<std::size_t>(n_) ? col_scale_[bv] : 1.0 / row_scale_[bv - static_cast<std::size_t>(n_)];
    for (const auto j : touched) {
      const auto sj = static_cast<std::size_t>(j);
      if (st_[sj] == VarStatus::Basic)
        continue;
      const double js = sj < static_cast<std::size_t>(n_) ? col_scale_[sj] : 1.0 / row_scale_[sj - static_cast<std::size_t>(n_)];
      row_out[sj] = row[sj] * bscale / js; // scaled row: x'_B + r'_j x'_j = 0 with x' = x / scale  =>  x_B + r'_j (s_B / s_j) x_j = 0
    }
  }
  [[nodiscard]] std::int32_t basic_variable(std::int32_t position) const { return basic_[static_cast<std::size_t>(position)]; }

  // Re-optimise after COST changes only (feasible basis stays primal feasible): primal simplex from the current basis.
  LpSolution resolve_primal() {
    if (!have_basis_)
      return solve();
    start_time_ = std::chrono::steady_clock::now();
    opt_.max_iterations = iterations_ + (per_solve_limit_ > 0 ? per_solve_limit_ : default_budget_);
    LpSolution out;
    if (need_refactor_)
      refactor();
    cost_ = orig_cost_;
    shifted_ = false;
    compute_primal();
    out.status = primal_loop(out);
    finish(out);
    return out;
  }

  // Warm re-solve after bound / cost changes: no initial perturbation, dual simplex from the current basis. Falls back to a cold
  // solve when the basis is not dual feasible any more.
  LpSolution resolve() {
    if (!have_basis_)
      return solve();
    start_time_ = std::chrono::steady_clock::now();
    opt_.max_iterations = iterations_ + (per_solve_limit_ > 0 ? per_solve_limit_ : default_budget_);
    LpSolution out;
    if (need_refactor_)
      refactor();
    cost_ = orig_cost_;
    shifted_ = false;
    compute_primal();
    compute_dual();
    if (!make_dual_feasible())
      return solve();
    compute_primal();
    const std::int64_t it0 = iterations_;
    SolveStatus s = dual_loop(out);
    if (s == SolveStatus::Optimal && shifted_)
      s = cleanup(out);
    out.status = s;
    finish(out);
    out.iterations = iterations_ - it0;
    return out;
  }

  [[nodiscard]] std::int32_t rows() const { return m_; }
  [[nodiscard]] std::int32_t cols() const { return n_; }

  LpSolution solve() {
    const auto t0 = std::chrono::steady_clock::now();
    start_time_ = t0;
    iterations_ = 0;
    opt_.max_iterations = default_budget_;
    LpSolution out;
    if (!have_basis_)
      slack_basis();
    refactor();
    compute_primal();
    compute_dual();
    // --- dual feasibility: phase I if needed
    shifted_ = false;
    if (opt_.perturb)
      perturb_costs();
    if (!make_dual_feasible()) {
      if (opt_.verbose)
        std::fprintf(stderr, "basis is dual infeasible: dual phase I\n");
      // dual phase I by the artificial-bounds subproblem
      if (!dual_phase_one(out)) {
        finish(out);
        return out;
      }
    }
    if (opt_.verbose)
      std::fprintf(stderr, "phase II dual loop start\n");
    SolveStatus s = dual_loop(out);
    if (opt_.verbose)
      std::fprintf(stderr, "dual loop ended with status %d after %lld iterations\n", static_cast<int>(s), static_cast<long long>(iterations_));
    if (s == SolveStatus::Optimal) {
      s = cleanup(out);
      if (s == SolveStatus::Optimal)
        polish(out);
    }
    out.status = s;
    finish(out);
    return out;
  }

private:
  // ------------------------------------------------------------------ setup
  void load(const Model &model) {
    model.validate();
    m_ = model.rows;
    n_ = model.cols;
    const auto nt = static_cast<std::size_t>(n_ + m_);
    col_scale_inv_.assign(static_cast<std::size_t>(n_), 1.0);
    row_scale_.assign(static_cast<std::size_t>(m_), 1.0);
    a_ = model.a;
    cost_.assign(nt, 0.0);
    lo_.assign(nt, 0.0);
    hi_.assign(nt, 0.0);
    std::vector<double> col_scale(static_cast<std::size_t>(n_), 1.0);
    if (opt_.scale)
      compute_scaling(col_scale);
    // apply scaling: A' = R A C, x = C x', rows l' = R l
    for (std::int32_t j = 0; j < n_; ++j) {
      col_scale_inv_[static_cast<std::size_t>(j)] = 1.0 / col_scale[static_cast<std::size_t>(j)];
      for (std::int64_t p = a_.start[static_cast<std::size_t>(j)]; p < a_.start[static_cast<std::size_t>(j) + 1]; ++p)
        a_.value[static_cast<std::size_t>(p)] *= row_scale_[static_cast<std::size_t>(a_.index[static_cast<std::size_t>(p)])] * col_scale[static_cast<std::size_t>(j)];
    }
    col_scale_ = col_scale;
    at_ = a_.transposed();
    for (std::int32_t j = 0; j < n_; ++j) {
      cost_[static_cast<std::size_t>(j)] = model.cost[static_cast<std::size_t>(j)] * col_scale[static_cast<std::size_t>(j)];
      lo_[static_cast<std::size_t>(j)] = std::isinf(model.col_lo[static_cast<std::size_t>(j)]) ? model.col_lo[static_cast<std::size_t>(j)] : model.col_lo[static_cast<std::size_t>(j)] / col_scale[static_cast<std::size_t>(j)];
      hi_[static_cast<std::size_t>(j)] = std::isinf(model.col_hi[static_cast<std::size_t>(j)]) ? model.col_hi[static_cast<std::size_t>(j)] : model.col_hi[static_cast<std::size_t>(j)] / col_scale[static_cast<std::size_t>(j)];
    }
    for (std::int32_t i = 0; i < m_; ++i) {
      const auto r = static_cast<std::size_t>(n_ + i);
      const double rs = row_scale_[static_cast<std::size_t>(i)];
      lo_[r] = std::isinf(model.row_lo[static_cast<std::size_t>(i)]) ? model.row_lo[static_cast<std::size_t>(i)] : model.row_lo[static_cast<std::size_t>(i)] * rs;
      hi_[r] = std::isinf(model.row_hi[static_cast<std::size_t>(i)]) ? model.row_hi[static_cast<std::size_t>(i)] : model.row_hi[static_cast<std::size_t>(i)] * rs;
    }
    orig_cost_ = cost_;
    orig_lo_ = lo_;
    orig_hi_ = hi_;
    offset_ = model.offset;
    st_.assign(nt, VarStatus::AtLower);
    x_.assign(nt, 0.0);
    d_.assign(nt, 0.0);
    basic_.assign(static_cast<std::size_t>(m_), 0);
    pos_.assign(nt, -1);
    dse_.assign(static_cast<std::size_t>(m_), 1.0);
    have_basis_ = false;
  }

  // geometric-mean scaling with power-of-two factors (rows then columns, a few passes)
  void compute_scaling(std::vector<double> &col_scale) {
    std::vector<double> rmin(static_cast<std::size_t>(m_)), rmax(static_cast<std::size_t>(m_));
    for (int pass = 0; pass < 4; ++pass) {
      std::fill(rmin.begin(), rmin.end(), kInf);
      std::fill(rmax.begin(), rmax.end(), 0.0);
      for (std::int32_t j = 0; j < n_; ++j)
        for (std::int64_t p = a_.start[static_cast<std::size_t>(j)]; p < a_.start[static_cast<std::size_t>(j) + 1]; ++p) {
          const auto i = static_cast<std::size_t>(a_.index[static_cast<std::size_t>(p)]);
          const double v = std::abs(a_.value[static_cast<std::size_t>(p)]) * row_scale_[i] * col_scale[static_cast<std::size_t>(j)];
          rmin[i] = std::min(rmin[i], v);
          rmax[i] = std::max(rmax[i], v);
        }
      for (std::int32_t i = 0; i < m_; ++i)
        if (rmax[static_cast<std::size_t>(i)] > 0.0)
          row_scale_[static_cast<std::size_t>(i)] /= std::sqrt(rmin[static_cast<std::size_t>(i)] * rmax[static_cast<std::size_t>(i)]);
      for (std::int32_t j = 0; j < n_; ++j) {
        double cmin = kInf, cmax = 0.0;
        for (std::int64_t p = a_.start[static_cast<std::size_t>(j)]; p < a_.start[static_cast<std::size_t>(j) + 1]; ++p) {
          const double v = std::abs(a_.value[static_cast<std::size_t>(p)]) * row_scale_[static_cast<std::size_t>(a_.index[static_cast<std::size_t>(p)])] * col_scale[static_cast<std::size_t>(j)];
          cmin = std::min(cmin, v);
          cmax = std::max(cmax, v);
        }
        if (cmax > 0.0)
          col_scale[static_cast<std::size_t>(j)] /= std::sqrt(cmin * cmax);
      }
    }
    auto pow2 = [](double v) { return std::exp2(std::round(std::log2(v))); };
    for (auto &v : row_scale_)
      v = pow2(v);
    for (auto &v : col_scale)
      v = pow2(v);
  }

  void slack_basis() {
    for (std::int32_t j = 0; j < n_; ++j)
      st_[static_cast<std::size_t>(j)] = VarStatus::AtLower;
    for (std::int32_t i = 0; i < m_; ++i) {
      st_[static_cast<std::size_t>(n_ + i)] = VarStatus::Basic;
      basic_[static_cast<std::size_t>(i)] = n_ + i;
    }
    std::fill(pos_.begin(), pos_.end(), -1);
    for (std::int32_t i = 0; i < m_; ++i)
      pos_[static_cast<std::size_t>(n_ + i)] = i;
    std::fill(dse_.begin(), dse_.end(), 1.0);
    place_nonbasic();
    have_basis_ = true;
    need_refactor_ = true;
  }

  // put every nonbasic variable on a bound consistent with its status (and finiteness)
  void place_nonbasic() {
    for (std::int32_t j = 0; j < n_ + m_; ++j) {
      const auto sj = static_cast<std::size_t>(j);
      if (st_[sj] == VarStatus::Basic)
        continue;
      const bool fl = !std::isinf(lo_[sj]), fu = !std::isinf(hi_[sj]);
      if (st_[sj] == VarStatus::AtUpper && fu) {
        x_[sj] = hi_[sj];
      } else if (st_[sj] == VarStatus::AtLower && fl) {
        x_[sj] = lo_[sj];
      } else if (fl) {
        st_[sj] = VarStatus::AtLower;
        x_[sj] = lo_[sj];
      } else if (fu) {
        st_[sj] = VarStatus::AtUpper;
        x_[sj] = hi_[sj];
      } else {
        st_[sj] = VarStatus::Free;
        x_[sj] = 0.0;
      }
    }
    primal_dirty_ = true;
  }

  // ------------------------------------------------------------------ basis factorisation
  void refactor() {
    const double t_start = now_s();
    ++tm_.refactors;
    struct Guard { Timers &t; double s; ~Guard() { t.refactor += now_s() - s; } } guard{tm_, t_start};
    for (int attempt = 0; attempt < 5; ++attempt) {
      std::vector<std::int64_t> start(1, 0);
      std::vector<std::int32_t> index;
      std::vector<double> value;
      for (std::int32_t k = 0; k < m_; ++k) {
        const auto j = basic_[static_cast<std::size_t>(k)];
        if (j < n_) {
          for (std::int64_t p = a_.start[static_cast<std::size_t>(j)]; p < a_.start[static_cast<std::size_t>(j) + 1]; ++p) {
            index.push_back(a_.index[static_cast<std::size_t>(p)]);
            value.push_back(a_.value[static_cast<std::size_t>(p)]);
          }
        } else {
          index.push_back(j - n_);
          value.push_back(-1.0);
        }
        start.push_back(static_cast<std::int64_t>(index.size()));
      }
      const auto sing = lu_.factorize(m_, start, index, value);
      if (sing.empty()) {
        need_refactor_ = false;
        since_refactor_ = 0;
        return;
      }
      for (const auto &s : sing) { // swap the dependent column for the logical of an uncovered row
        const auto out_var = basic_[static_cast<std::size_t>(s.position)];
        const auto in_var = n_ + s.row;
        st_[static_cast<std::size_t>(out_var)] = VarStatus::AtLower;
        pos_[static_cast<std::size_t>(out_var)] = -1;
        basic_[static_cast<std::size_t>(s.position)] = in_var;
        st_[static_cast<std::size_t>(in_var)] = VarStatus::Basic;
        pos_[static_cast<std::size_t>(in_var)] = s.position;
        dse_[static_cast<std::size_t>(s.position)] = 1.0;
      }
      place_nonbasic();
    }
    throw std::runtime_error("lp::Simplex: could not repair a singular basis");
  }

  // ------------------------------------------------------------------ primal / dual values
  void compute_primal() {
    std::vector<double> rhs(static_cast<std::size_t>(m_), 0.0);
    for (std::int32_t j = 0; j < n_ + m_; ++j) {
      if (st_[static_cast<std::size_t>(j)] == VarStatus::Basic)
        continue;
      const double xj = x_[static_cast<std::size_t>(j)];
      if (xj == 0.0)
        continue;
      if (j < n_) {
        for (std::int64_t p = a_.start[static_cast<std::size_t>(j)]; p < a_.start[static_cast<std::size_t>(j) + 1]; ++p)
          rhs[static_cast<std::size_t>(a_.index[static_cast<std::size_t>(p)])] -= a_.value[static_cast<std::size_t>(p)] * xj;
      } else {
        rhs[static_cast<std::size_t>(j - n_)] += xj;
      }
    }
    lu_.ftran(rhs);
    for (std::int32_t k = 0; k < m_; ++k)
      x_[static_cast<std::size_t>(basic_[static_cast<std::size_t>(k)])] = rhs[static_cast<std::size_t>(k)];
    primal_dirty_ = false;
  }

  void compute_dual() {
    std::vector<double> y(static_cast<std::size_t>(m_));
    for (std::int32_t k = 0; k < m_; ++k)
      y[static_cast<std::size_t>(k)] = cost_[static_cast<std::size_t>(basic_[static_cast<std::size_t>(k)])];
    lu_.btran(y);
    y_ = y;
    for (std::int32_t j = 0; j < n_ + m_; ++j) {
      if (st_[static_cast<std::size_t>(j)] == VarStatus::Basic) {
        d_[static_cast<std::size_t>(j)] = 0.0;
        continue;
      }
      if (j < n_) {
        double s = cost_[static_cast<std::size_t>(j)];
        for (std::int64_t p = a_.start[static_cast<std::size_t>(j)]; p < a_.start[static_cast<std::size_t>(j) + 1]; ++p)
          s -= a_.value[static_cast<std::size_t>(p)] * y[static_cast<std::size_t>(a_.index[static_cast<std::size_t>(p)])];
        d_[static_cast<std::size_t>(j)] = s;
      } else {
        d_[static_cast<std::size_t>(j)] = cost_[static_cast<std::size_t>(j)] + y[static_cast<std::size_t>(j - n_)];
      }
    }
  }

  // ------------------------------------------------------------------ dual feasibility
  // Place boxed variables on the bound that makes d_j feasible; report whether every variable is dual feasible.
  bool make_dual_feasible() {
    bool ok = true;
    for (std::int32_t j = 0; j < n_ + m_; ++j) {
      const auto sj = static_cast<std::size_t>(j);
      if (st_[sj] == VarStatus::Basic)
        continue;
      const bool fl = !std::isinf(lo_[sj]), fu = !std::isinf(hi_[sj]);
      const double dj = d_[sj];
      if (fl && fu) {
        if (lo_[sj] == hi_[sj]) {
          st_[sj] = VarStatus::AtLower;
          x_[sj] = lo_[sj];
        } else if (dj >= 0.0) {
          st_[sj] = VarStatus::AtLower;
          x_[sj] = lo_[sj];
        } else {
          st_[sj] = VarStatus::AtUpper;
          x_[sj] = hi_[sj];
        }
      } else if (fl) {
        st_[sj] = VarStatus::AtLower;
        x_[sj] = lo_[sj];
        if (dj < -opt_.dual_tol)
          ok = false;
      } else if (fu) {
        st_[sj] = VarStatus::AtUpper;
        x_[sj] = hi_[sj];
        if (dj > opt_.dual_tol)
          ok = false;
      } else {
        st_[sj] = VarStatus::Free;
        x_[sj] = 0.0;
        if (std::abs(dj) > opt_.dual_tol)
          ok = false;
      }
    }
    primal_dirty_ = true;
    return ok;
  }

  void perturb_costs() {
    // Perturbation must clearly exceed the dual tolerance or ties are never broken (Harris windows swallow them) and the dual
    // simplex cycles. Scale: base * (1 + |c_j|) * (1 + rand), with a base growing with the cost range as in HiGHS.
    std::mt19937_64 rng(12345);
    std::uniform_real_distribution<double> u(0.0, 1.0);
    double cmax = 0.0;
    for (std::int32_t j = 0; j < n_; ++j)
      cmax = std::max(cmax, std::abs(cost_[static_cast<std::size_t>(j)]));
    double base = 5e-6;
    if (cmax > 100.0)
      base *= std::sqrt(std::sqrt(cmax / 100.0));
    for (std::int32_t j = 0; j < n_ + m_; ++j) {
      const auto sj = static_cast<std::size_t>(j);
      if (lo_[sj] == hi_[sj] && !std::isinf(lo_[sj]))
        continue;
      const double mag = base * (1.0 + std::abs(cost_[sj])) * (1.0 + u(rng));
      const bool fl = !std::isinf(lo_[sj]), fu = !std::isinf(hi_[sj]);
      if (fl && !fu)
        cost_[sj] += mag;   // lower-bounded: keep d_j >= 0 comfortably
      else if (fu && !fl)
        cost_[sj] -= mag;   // upper-bounded: keep d_j <= 0
      else if (fl && fu)
        cost_[sj] += (std::abs(cost_[sj]) < 1e-12 || cost_[sj] > 0 ? 1.0 : -1.0) * mag; // boxed: push away from zero
    }
    shifted_ = true;
    compute_dual();
  }

  // Stall response: add a fresh, larger random perturbation to the NONBASIC costs (direction chosen to keep d_j feasible).
  void escalate_perturbation(int level) {
    thread_local std::mt19937_64 rng(777);
    std::uniform_real_distribution<double> u(0.0, 1.0);
    const double base = 5e-6 * std::pow(4.0, level);
    for (std::int32_t j = 0; j < n_ + m_; ++j) {
      const auto sj = static_cast<std::size_t>(j);
      if (st_[sj] == VarStatus::Basic || (lo_[sj] == hi_[sj] && !std::isinf(lo_[sj])))
        continue;
      const double mag = base * (1.0 + std::abs(cost_[sj])) * (1.0 + u(rng));
      if (st_[sj] == VarStatus::AtLower) {
        cost_[sj] += mag;
        d_[sj] += mag;
      } else if (st_[sj] == VarStatus::AtUpper) {
        cost_[sj] -= mag;
        d_[sj] -= mag;
      }
    }
    shifted_ = true;
    if (opt_.verbose)
      std::fprintf(stderr, "  stall at iteration %lld: perturbation escalated to level %d\n", static_cast<long long>(iterations_), level);
  }

  // dual phase I: solve the box-relaxed problem with the dual simplex, then restore the true bounds.
  bool dual_phase_one(LpSolution &out) {
    // artificial bounds
    for (std::int32_t j = 0; j < n_ + m_; ++j) {
      const auto sj = static_cast<std::size_t>(j);
      const bool fl = !std::isinf(orig_lo_[sj]), fu = !std::isinf(orig_hi_[sj]);
      if (fl && fu) {
        lo_[sj] = 0.0;
        hi_[sj] = 0.0;
      } else if (fl) {
        lo_[sj] = 0.0;
        hi_[sj] = 1.0;
      } else if (fu) {
        lo_[sj] = -1.0;
        hi_[sj] = 0.0;
      } else {
        lo_[sj] = -1000.0;
        hi_[sj] = 1000.0;
      }
    }
    place_nonbasic();
    make_dual_feasible();
    compute_primal();
    LpSolution scratch;
    const SolveStatus s = dual_loop(scratch);
    // restore
    lo_ = orig_lo_;
    hi_ = orig_hi_;
    place_nonbasic();
    compute_dual();
    const bool feasible = make_dual_feasible();
    compute_primal();
    if (opt_.verbose) {
      int bad_x = 0, bad_d = 0, neg_d = 0;
      for (std::size_t j = 0; j < x_.size(); ++j) {
        bad_x += !std::isfinite(x_[j]);
        bad_d += !std::isfinite(d_[j]);
        neg_d += st_[j] != VarStatus::Basic && ((st_[j] == VarStatus::AtLower && d_[j] < -opt_.dual_tol) || (st_[j] == VarStatus::AtUpper && d_[j] > opt_.dual_tol));
      }
      std::fprintf(stderr, "after phase I: status %d feasible=%d nonfinite x=%d d=%d dual-infeasible=%d\n", static_cast<int>(s), feasible, bad_x, bad_d, neg_d);
    }
    if (s != SolveStatus::Optimal && s != SolveStatus::Infeasible) {
      out.status = s;
      return false;
    }
    if (feasible)
      return true;
    // dual infeasible: the LP is unbounded or infeasible. Decide with the zero-cost problem.
    return resolve_dual_infeasible(out);
  }

  // The true costs make the basis dual infeasible. Zero costs always give a dual feasible start: if that problem is primal
  // infeasible so is the LP; otherwise run the primal simplex with the true costs from the feasible basis it finds.
  bool resolve_dual_infeasible(LpSolution &out) {
    cost_.assign(cost_.size(), 0.0);
    compute_dual();
    make_dual_feasible();
    compute_primal();
    LpSolution scratch;
    const SolveStatus s = dual_loop(scratch);
    if (s == SolveStatus::Infeasible) {
      out.status = SolveStatus::Infeasible;
      out.farkas = scratch.farkas;
      return false;
    }
    cost_ = orig_cost_;
    shifted_ = false;
    compute_dual();
    if (opt_.verbose)
      std::fprintf(stderr, "zero-cost dual loop status %d; starting primal loop with true costs\n", static_cast<int>(s));
    const SolveStatus ps = primal_loop(out);
    if (opt_.verbose)
      std::fprintf(stderr, "primal loop status %d after %lld iterations\n", static_cast<int>(ps), static_cast<long long>(iterations_));
    out.status = ps;
    if (ps == SolveStatus::Optimal)
      return false; // finish() reports; main solve() returns early through the status already set
    return false;
  }

  // ------------------------------------------------------------------ dual simplex
  SolveStatus dual_loop(LpSolution &out) {
    std::vector<double> rho(static_cast<std::size_t>(m_)), alpha_q(static_cast<std::size_t>(m_)), tau(static_cast<std::size_t>(m_)), row(static_cast<std::size_t>(n_ + m_));
    std::vector<std::int32_t> touched;
    std::fill(row.begin(), row.end(), 0.0);
    std::vector<std::int32_t> rho_nz, alpha_nz, tau_nz, frhs_nz, alpha_hint;
    std::vector<double> frhs(static_cast<std::size_t>(m_), 0.0);
    std::fill(tau.begin(), tau.end(), 0.0);
    // squared primal infeasibility per basis position (0 when within tolerance), maintained incrementally so CHUZR scans two
    // contiguous arrays instead of chasing basic_ -> x_/lo_/hi_
    std::vector<double> pinf(static_cast<std::size_t>(m_), 0.0);
    bool pinf_ok = false;
    std::vector<std::int32_t> inf_list;      // positions with pinf > 0 (may hold stale zero entries; purged by CHUZR)
    std::vector<char> in_list(static_cast<std::size_t>(m_), 0);
    const auto set_pinf = [&](std::size_t k, double v) {
      pinf[k] = v;
      if (v > 0.0 && !in_list[k]) {
        in_list[k] = 1;
        inf_list.push_back(static_cast<std::int32_t>(k));
      }
    };
    const auto pinf_of = [&](std::size_t k) {
      const auto j = static_cast<std::size_t>(basic_[k]);
      const double xv = x_[j];
      double inf = 0.0;
      if (xv < lo_[j] - opt_.primal_tol)
        inf = lo_[j] - xv;
      else if (xv > hi_[j] + opt_.primal_tol)
        inf = xv - hi_[j];
      return inf * inf;
    };
    // adaptive refactorisation: iterations get slower as eta columns pile up; refactor once an iteration costs more than the average
    // cost per iteration since the last refactorisation (including the refactorisation itself)
    double refactor_block_s = 0.0, acc_iter_s = 0.0, ema_iter_s = 0.0, iter_t0 = now_s();
    int degenerate = 0;
    double best_obj = -kInf;
    std::int64_t best_it = iterations_;
    int escalations = 0;
    while (true) {
      if (iterations_ >= opt_.max_iterations)
        return SolveStatus::IterationLimit;
      if (iterations_ - best_it >= 300 && (iterations_ % 100) == 0) { // stall: the dual objective must be non-decreasing
        double obj = 0.0;
        for (std::int32_t j = 0; j < n_ + m_; ++j)
          obj += cost_[static_cast<std::size_t>(j)] * x_[static_cast<std::size_t>(j)];
        if (obj > best_obj + 1e-9 * (1.0 + std::abs(best_obj))) {
          best_obj = obj;
          best_it = iterations_;
        } else if (escalations < 12) {
          escalate_perturbation(++escalations);
          best_it = iterations_;
        }
      } else if (iterations_ == best_it) {
        double obj = 0.0;
        for (std::int32_t j = 0; j < n_ + m_; ++j)
          obj += cost_[static_cast<std::size_t>(j)] * x_[static_cast<std::size_t>(j)];
        best_obj = std::max(best_obj, obj);
      }
      if ((iterations_ & 63) == 0 && (elapsed() > opt_.time_limit_seconds || (opt_.stop && opt_.stop->load(std::memory_order_relaxed))))
        return SolveStatus::TimeLimit;
      const bool adaptive_due = since_refactor_ >= 25 && ema_iter_s > 1.05 * (refactor_block_s + acc_iter_s) / static_cast<double>(since_refactor_);
      if (need_refactor_ || adaptive_due || since_refactor_ >= opt_.refactor_interval || lu_.eta_nnz() > 2 * lu_.factor_nnz() + 10 * m_) {
        const double rb0 = now_s();
        refactor();
        compute_primal();
        compute_dual();
        fix_dual_infeasibilities();
        pinf_ok = false;
        refactor_block_s = now_s() - rb0;
        acc_iter_s = 0.0;
        ema_iter_s = 0.0;
      }
      if (primal_dirty_) {
        compute_primal();
        pinf_ok = false;
      }
      if (!std::isinf(cutoff_scaled_) && (iterations_ % 10) == 0) { // dual objective is monotone: prune once it exceeds the cutoff
        double obj = 0.0;
        for (std::int32_t j = 0; j < n_; ++j)
          obj += cost_[static_cast<std::size_t>(j)] * x_[static_cast<std::size_t>(j)];
        if (obj > cutoff_scaled_ + 1e-6 * (1.0 + std::abs(cutoff_scaled_)))
          return SolveStatus::ObjectiveLimit;
      }
      // ---- CHUZR
      const double t_chuzr = now_s();
      std::int32_t r = -1;
      double best = 0.0;
      if (!pinf_ok) {
        inf_list.clear();
        std::fill(in_list.begin(), in_list.end(), 0);
        for (std::size_t k = 0; k < static_cast<std::size_t>(m_); ++k) {
          pinf[k] = 0.0;
          set_pinf(k, pinf_of(k));
        }
        pinf_ok = true;
      }
      for (std::size_t t = 0; t < inf_list.size();) {
        const auto k = static_cast<std::size_t>(inf_list[t]);
        const double pk = pinf[k];
        if (pk <= 0.0) { // feasible again: drop from the list
          in_list[k] = 0;
          inf_list[t] = inf_list.back();
          inf_list.pop_back();
          continue;
        }
        const double score = pk / dse_[k];
        if (score > best || (score == best && static_cast<std::int32_t>(k) < r)) { // lowest index on ties: independent of list order
          best = score;
          r = static_cast<std::int32_t>(k);
        }
        ++t;
      }
      if (opt_.verbose && (iterations_ % 1000) == 0) {
        double sum = 0.0;
        std::int32_t cnt = 0;
        for (std::int32_t k = 0; k < m_; ++k) {
          const auto j = static_cast<std::size_t>(basic_[static_cast<std::size_t>(k)]);
          const double v = std::max({0.0, lo_[j] - x_[j], x_[j] - hi_[j]});
          if (v > opt_.primal_tol) {
            sum += v;
            ++cnt;
          }
        }
        double obj = 0.0;
        for (std::int32_t j = 0; j < n_; ++j)
          obj += cost_[static_cast<std::size_t>(j)] * x_[static_cast<std::size_t>(j)];
        std::fprintf(stderr, "  it %lld: primal infeas sum %.4e (%d rows) obj %.10g eta %zu LUnnz %lld\n", static_cast<long long>(iterations_), sum, cnt, obj, lu_.eta_count(), static_cast<long long>(lu_.factor_nnz()));
      }
      tm_.chuzr += now_s() - t_chuzr;
      if (r < 0)
        return SolveStatus::Optimal;
      const auto p_var = static_cast<std::size_t>(basic_[static_cast<std::size_t>(r)]);
      const double sigma = x_[p_var] < lo_[p_var] ? 1.0 : -1.0;       // +1: leaves at its lower bound
      const double leave_bound = sigma > 0 ? lo_[p_var] : hi_[p_var];
      double delta = x_[p_var] - leave_bound;                        // signed primal infeasibility
      // ---- BTRAN rho = e_r B^{-1}, pivot row
      double tt = now_s();
      std::fill(rho.begin(), rho.end(), 0.0);
      rho[static_cast<std::size_t>(r)] = 1.0;
      lu_.btran(rho);
      tm_.btran += now_s() - tt;
      tt = now_s();
      rho_nz.clear();
      double rho_norm2 = 0.0;
      for (std::size_t i = 0; i < static_cast<std::size_t>(m_); ++i) {
        const double v = rho[i];
        if (v != 0.0) {
          rho_nz.push_back(static_cast<std::int32_t>(i));
          rho_norm2 += v * v;
        }
      }
      dse_[static_cast<std::size_t>(r)] = rho_norm2;
      compute_pivot_row(rho, row, touched, &rho_nz);
      tm_.row += now_s() - tt;
      tt = now_s();
      // ---- CHUZC with bound flipping
      struct Cand {
        std::int32_t j;
        double ratio;
        double atilde;
      };
      std::vector<Cand> cands;
      for (const auto j : touched) {
        const auto sj = static_cast<std::size_t>(j);
        if (st_[sj] == VarStatus::Basic)
          continue;
        if (lo_[sj] == hi_[sj] && !std::isinf(lo_[sj]))
          continue; // fixed
        const double at = -sigma * row[sj];
        if (std::abs(at) < opt_.pivot_tol * 1e-3)
          continue;
        bool ok = false;
        if (st_[sj] == VarStatus::AtLower && at > 0)
          ok = true;
        else if (st_[sj] == VarStatus::AtUpper && at < 0)
          ok = true;
        else if (st_[sj] == VarStatus::Free)
          ok = true;
        if (!ok)
          continue;
        cands.push_back({j, std::max(0.0, std::abs(d_[sj])) / std::abs(at), at});
      }
      if (cands.empty()) {
        out.farkas = rho; // proof of primal infeasibility: row multipliers
        out.ray.clear();
        return SolveStatus::Infeasible;
      }
      // candidates are only sorted as far as the ratio test walks: the bound-flipping walk and the Harris pass usually stop after a few
      std::size_t sorted_count = 0;
      const auto need = [&](std::size_t upto) {
        upto = std::min(upto, cands.size());
        if (upto <= sorted_count)
          return;
        const std::size_t target = std::min(cands.size(), std::max<std::size_t>(upto, std::max<std::size_t>(32, 2 * sorted_count)));
        std::partial_sort(cands.begin(), cands.begin() + static_cast<std::ptrdiff_t>(target), cands.end(), [](const Cand &a, const Cand &b) { return a.ratio < b.ratio; });
        sorted_count = target;
      };
      // bound-flipping: walk breakpoints while the slope stays non-negative
      double slope = std::abs(delta);
      std::size_t kstar = cands.size();
      for (std::size_t t = 0; t < cands.size(); ++t) {
        need(t + 1);
        const auto sj = static_cast<std::size_t>(cands[t].j);
        const double range = hi_[sj] - lo_[sj];
        if (std::isinf(range) || st_[sj] == VarStatus::Free) {
          kstar = t;
          break;
        }
        const double next_slope = slope - std::abs(row[sj]) * range;
        if (next_slope < 0.0) {
          kstar = t;
          break;
        }
        slope = next_slope;
      }
      if (kstar == cands.size()) {
        if (slope > opt_.primal_tol) { // even flipping every boxed candidate cannot repair this row: primal infeasible
          out.farkas = rho;
          return SolveStatus::Infeasible;
        }
        kstar = cands.size() - 1; // marginal: the last candidate enters
      }
      // Harris selection among the remaining candidates (ratio window)
      double window = kInf;
      for (std::size_t t = kstar; t < cands.size(); ++t)
        window = std::min(window, (std::abs(d_[static_cast<std::size_t>(cands[t].j)]) + opt_.dual_tol) / std::abs(cands[t].atilde));
      std::size_t pick = kstar;
      double pick_abs = 0.0;
      for (std::size_t t = kstar; t < cands.size(); ++t) {
        need(t + 1);
        if (!(cands[t].ratio <= window))
          break;
        if (std::abs(cands[t].atilde) > pick_abs) {
          pick_abs = std::abs(cands[t].atilde);
          pick = t;
        }
      }
      const std::int32_t q = cands[pick].j;
      const double theta = cands[pick].ratio;
      if (opt_.verbose)
        if (false) std::fprintf(stderr, "dual it %lld: r=%d (var %d x=%.9g [%.6g,%.6g] dse=%.4g) sigma=%g delta=%.9g -> q=%d (x=%.9g [%.6g,%.6g] d=%.3e row=%.6g) theta=%.3e flips=%zu cands=%zu\n", static_cast<long long>(iterations_), r, static_cast<int>(p_var), x_[p_var], lo_[p_var], hi_[p_var], dse_[static_cast<std::size_t>(r)], sigma, delta, q, x_[static_cast<std::size_t>(q)], lo_[static_cast<std::size_t>(q)], hi_[static_cast<std::size_t>(q)], d_[static_cast<std::size_t>(q)], row[static_cast<std::size_t>(q)], theta, pick, cands.size());
      if (std::abs(row[static_cast<std::size_t>(q)]) < opt_.pivot_tol) {
        // unstable pivot: refactor once to clean the factors, otherwise accept
        if (since_refactor_ > 0) {
          need_refactor_ = true;
          continue;
        }
      }
      tm_.chuzc += now_s() - tt;
      tt = now_s();
      // ---- FTRAN of the entering column and consistency check BEFORE any state is modified
      for (const auto i : alpha_nz)
        alpha_q[static_cast<std::size_t>(i)] = 0.0;
      scatter_column(q, alpha_q);
      alpha_hint.clear();
      if (q < n_)
        alpha_hint.assign(a_.index.begin() + a_.start[static_cast<std::size_t>(q)], a_.index.begin() + a_.start[static_cast<std::size_t>(q) + 1]);
      else
        alpha_hint.push_back(q - n_);
      lu_.ftran(alpha_q, &alpha_nz, &alpha_hint);
      const double alpha_rq = alpha_q[static_cast<std::size_t>(r)];
      const double row_rq = row[static_cast<std::size_t>(q)];
      if (std::abs(alpha_rq) < 1e-9 || std::abs(alpha_rq - row_rq) > 1e-6 * (1.0 + std::abs(alpha_rq))) {
        if (since_refactor_ > 0) { // numerically unreliable pivot: refactor and retry
          need_refactor_ = true;
          continue;
        }
      }
      // flip every boxed candidate before the entering one in ratio order
      std::vector<std::int32_t> flips;
      for (std::size_t t = 0; t < pick; ++t) {
        const auto j = cands[t].j;
        const auto sj = static_cast<std::size_t>(j);
        if (j != q && !std::isinf(hi_[sj] - lo_[sj]) && cands[t].ratio < theta - 1e-15)
          flips.push_back(j);
      }
      if (!flips.empty()) {
        for (const auto i : frhs_nz)
          frhs[static_cast<std::size_t>(i)] = 0.0;
        for (const auto j : flips) {
          const auto sj = static_cast<std::size_t>(j);
          const double dx = st_[sj] == VarStatus::AtLower ? hi_[sj] - lo_[sj] : lo_[sj] - hi_[sj];
          scatter_column_scaled(j, dx, frhs);
          x_[sj] = st_[sj] == VarStatus::AtLower ? hi_[sj] : lo_[sj];
          st_[sj] = st_[sj] == VarStatus::AtLower ? VarStatus::AtUpper : VarStatus::AtLower;
        }
        lu_.ftran(frhs, &frhs_nz);
        for (const auto k : frhs_nz) {
          const double fk = frhs[static_cast<std::size_t>(k)];
          if (fk == 0.0)
            continue;
          x_[static_cast<std::size_t>(basic_[static_cast<std::size_t>(k)])] -= fk;
          set_pinf(static_cast<std::size_t>(k), pinf_of(static_cast<std::size_t>(k)));
        }
        delta = x_[p_var] - leave_bound;
      }
      // ---- DSE: tau = B^{-1} rho
      for (const auto i : tau_nz)
        tau[static_cast<std::size_t>(i)] = 0.0;
      for (const auto i : rho_nz)
        tau[static_cast<std::size_t>(i)] = rho[static_cast<std::size_t>(i)];
      lu_.ftran(tau, &tau_nz, &rho_nz);
      tm_.ftran += now_s() - tt;
      tt = now_s();
      // ---- update reduced costs (only entries with a non-zero pivot-row element change)
      const double sgn = sigma;
      for (const auto j : touched) {
        const auto sj = static_cast<std::size_t>(j);
        if (st_[sj] == VarStatus::Basic || j == q)
          continue;
        d_[sj] += sgn * theta * row[sj];
        // cost shifting for Harris-induced dual infeasibilities
        const double dj = d_[sj];
        if (((st_[sj] == VarStatus::AtLower && dj < 0.0) || (st_[sj] == VarStatus::AtUpper && dj > 0.0)) && !(lo_[sj] == hi_[sj]) && std::abs(dj) <= 10 * opt_.dual_tol) {
          cost_[sj] -= dj;
          d_[sj] = 0.0;
          shifted_ = true;
        }
      }
      d_[p_var] = sgn * theta;
      d_[static_cast<std::size_t>(q)] = 0.0;
      const double t_pu = now_s();
      // ---- primal update
      const double dxq = delta / alpha_rq;
      for (const auto k : alpha_nz) {
        const double ak = alpha_q[static_cast<std::size_t>(k)];
        if (ak == 0.0)
          continue;
        x_[static_cast<std::size_t>(basic_[static_cast<std::size_t>(k)])] -= dxq * ak;
        set_pinf(static_cast<std::size_t>(k), pinf_of(static_cast<std::size_t>(k)));
      }
      x_[static_cast<std::size_t>(q)] += dxq;
      x_[p_var] = leave_bound;
      // ---- DSE weights
      const double wr = rho_norm2;
      for (const auto k : alpha_nz) {
        if (k == r)
          continue;
        const double ratio = alpha_q[static_cast<std::size_t>(k)] / alpha_rq;
        if (ratio == 0.0)
          continue;
        dse_[static_cast<std::size_t>(k)] = std::max(dse_[static_cast<std::size_t>(k)] - 2.0 * ratio * tau[static_cast<std::size_t>(k)] + ratio * ratio * wr, 1e-4);
      }
      dse_[static_cast<std::size_t>(r)] = std::max(wr / (alpha_rq * alpha_rq), 1e-4);
      tm_.dse += now_s() - t_pu;
      // ---- basis change
      st_[p_var] = sigma > 0 ? VarStatus::AtLower : VarStatus::AtUpper;
      pos_[p_var] = -1;
      basic_[static_cast<std::size_t>(r)] = q;
      st_[static_cast<std::size_t>(q)] = VarStatus::Basic;
      pos_[static_cast<std::size_t>(q)] = r;
      set_pinf(static_cast<std::size_t>(r), pinf_of(static_cast<std::size_t>(r)));
      lu_.update(r, alpha_q, &alpha_nz);
      tm_.update += now_s() - tt;
      ++since_refactor_;
      ++iterations_;
      degenerate = theta < 1e-12 ? degenerate + 1 : 0;
      (void)degenerate;
      {
        const double now = now_s();
        const double dt = now - iter_t0;
        iter_t0 = now;
        acc_iter_s += dt;
        ema_iter_s = ema_iter_s == 0.0 ? dt : 0.9 * ema_iter_s + 0.1 * dt;
      }
    }
  }

  // repair d_j signs after a refactor (shifting costs of small violations, flipping boxed variables otherwise)
  void fix_dual_infeasibilities() {
    for (std::int32_t j = 0; j < n_ + m_; ++j) {
      const auto sj = static_cast<std::size_t>(j);
      if (st_[sj] == VarStatus::Basic || (lo_[sj] == hi_[sj] && !std::isinf(lo_[sj])))
        continue;
      const double dj = d_[sj];
      const bool fl = !std::isinf(lo_[sj]), fu = !std::isinf(hi_[sj]);
      if (st_[sj] == VarStatus::AtLower && dj < -opt_.dual_tol) {
        if (fu) {
          st_[sj] = VarStatus::AtUpper;
          x_[sj] = hi_[sj];
          primal_dirty_ = true;
        } else {
          cost_[sj] -= dj;
          d_[sj] = 0.0;
          shifted_ = true;
        }
      } else if (st_[sj] == VarStatus::AtUpper && dj > opt_.dual_tol) {
        if (fl) {
          st_[sj] = VarStatus::AtLower;
          x_[sj] = lo_[sj];
          primal_dirty_ = true;
        } else {
          cost_[sj] -= dj;
          d_[sj] = 0.0;
          shifted_ = true;
        }
      }
    }
  }

  // pivot row alpha_r = rho^T [A | -I] over all variables; `row` is a persistent zero vector, `touched` lists its non-zero entries
  void compute_pivot_row(const std::vector<double> &rho, std::vector<double> &row, std::vector<std::int32_t> &touched, const std::vector<std::int32_t> *rho_nz = nullptr) {
    for (const auto j : touched)
      row[static_cast<std::size_t>(j)] = 0.0;
    touched.clear();
    if (mark_.size() != static_cast<std::size_t>(n_ + m_))
      mark_.assign(static_cast<std::size_t>(n_ + m_), 0);
    // a fresh stamp per call: duplicate suppression needs no clean-up and cannot leak between solver loops
    if (++stamp_ == 0) {
      std::fill(mark_.begin(), mark_.end(), 0);
      stamp_ = 1;
    }
    const std::size_t count = rho_nz ? rho_nz->size() : static_cast<std::size_t>(m_);
    for (std::size_t it = 0; it < count; ++it) {
      const std::int32_t i = rho_nz ? (*rho_nz)[it] : static_cast<std::int32_t>(it);
      const double r = rho[static_cast<std::size_t>(i)];
      if (r == 0.0)
        continue;
      for (std::int64_t p = at_.start[static_cast<std::size_t>(i)]; p < at_.start[static_cast<std::size_t>(i) + 1]; ++p) {
        const auto j = static_cast<std::size_t>(at_.index[static_cast<std::size_t>(p)]);
        if (mark_[j] != stamp_) { // exact cancellations (integer data) must not create duplicate list entries
          mark_[j] = stamp_;
          touched.push_back(static_cast<std::int32_t>(j));
        }
        row[j] += r * at_.value[static_cast<std::size_t>(p)];
      }
      row[static_cast<std::size_t>(n_ + i)] = -r; // logical column is -e_i
      touched.push_back(n_ + i);
    }
  }

  void scatter_column(std::int32_t j, std::vector<double> &v) const { scatter_column_scaled(j, 1.0, v); }
  void scatter_column_scaled(std::int32_t j, double s, std::vector<double> &v) const {
    if (j < n_) {
      for (std::int64_t p = a_.start[static_cast<std::size_t>(j)]; p < a_.start[static_cast<std::size_t>(j) + 1]; ++p)
        v[static_cast<std::size_t>(a_.index[static_cast<std::size_t>(p)])] += s * a_.value[static_cast<std::size_t>(p)];
    } else {
      v[static_cast<std::size_t>(j - n_)] -= s;
    }
  }

  // ------------------------------------------------------------------ primal simplex (Devex)
  SolveStatus primal_loop(LpSolution &out) {
    std::vector<double> alpha_q(static_cast<std::size_t>(m_)), rho(static_cast<std::size_t>(m_)), row(static_cast<std::size_t>(n_ + m_), 0.0);
    std::vector<std::int32_t> touched;
    std::vector<double> devex(static_cast<std::size_t>(n_ + m_), 1.0);
    if (need_refactor_) {
      refactor();
      compute_dual();
    }
    compute_primal();
    compute_dual();
    while (true) {
      if (iterations_ >= opt_.max_iterations)
        return SolveStatus::IterationLimit;
      if ((iterations_ & 63) == 0 && (elapsed() > opt_.time_limit_seconds || (opt_.stop && opt_.stop->load(std::memory_order_relaxed))))
        return SolveStatus::TimeLimit;
      if (since_refactor_ >= opt_.refactor_interval) {
        refactor();
        compute_primal();
        compute_dual();
      }
      // ---- is the current basis primal feasible? (the primal loop assumes it is; infeasibilities are repaired by the caller)
      // ---- CHUZC: largest scaled dual infeasibility
      std::int32_t q = -1;
      double best = 0.0;
      double dir = 0.0;
      for (std::int32_t j = 0; j < n_ + m_; ++j) {
        const auto sj = static_cast<std::size_t>(j);
        if (st_[sj] == VarStatus::Basic || (lo_[sj] == hi_[sj] && !std::isinf(lo_[sj])))
          continue;
        const double dj = d_[sj];
        double viol = 0.0, dj_dir = 0.0;
        if ((st_[sj] == VarStatus::AtLower || st_[sj] == VarStatus::Free) && dj < -opt_.dual_tol) {
          viol = -dj;
          dj_dir = 1.0;
        } else if ((st_[sj] == VarStatus::AtUpper || st_[sj] == VarStatus::Free) && dj > opt_.dual_tol) {
          viol = dj;
          dj_dir = -1.0;
        }
        if (viol > 0.0) {
          const double score = viol * viol / devex[sj];
          if (score > best) {
            best = score;
            q = j;
            dir = dj_dir;
          }
        }
      }
      if (q < 0)
        return SolveStatus::Optimal;
      std::fill(alpha_q.begin(), alpha_q.end(), 0.0);
      scatter_column(q, alpha_q);
      lu_.ftran(alpha_q);
      // ---- ratio test: x_B(t) = x_B - t * dir * alpha_q
      const auto sq = static_cast<std::size_t>(q);
      double tmax = std::isinf(hi_[sq] - lo_[sq]) ? kInf : hi_[sq] - lo_[sq];
      // Harris pass 1
      double relaxed = kInf;
      for (std::int32_t k = 0; k < m_; ++k) {
        const double a = dir * alpha_q[static_cast<std::size_t>(k)];
        if (std::abs(a) < opt_.pivot_tol)
          continue;
        const auto j = static_cast<std::size_t>(basic_[static_cast<std::size_t>(k)]);
        if (a > 0.0 && !std::isinf(lo_[j]))
          relaxed = std::min(relaxed, (x_[j] - lo_[j] + opt_.primal_tol) / a);
        else if (a < 0.0 && !std::isinf(hi_[j]))
          relaxed = std::min(relaxed, (hi_[j] - x_[j] + opt_.primal_tol) / (-a));
      }
      relaxed = std::max(relaxed, 0.0); // a basic variable already beyond its bound by more than the tolerance: step 0, never negative
      std::int32_t leave = -1;
      double leave_abs = 0.0, t_step = std::min(tmax, relaxed);
      for (std::int32_t k = 0; k < m_; ++k) {
        const double a = dir * alpha_q[static_cast<std::size_t>(k)];
        if (std::abs(a) < opt_.pivot_tol)
          continue;
        const auto j = static_cast<std::size_t>(basic_[static_cast<std::size_t>(k)]);
        double ratio = kInf;
        if (a > 0.0 && !std::isinf(lo_[j]))
          ratio = std::max(0.0, x_[j] - lo_[j]) / a;
        else if (a < 0.0 && !std::isinf(hi_[j]))
          ratio = std::max(0.0, hi_[j] - x_[j]) / (-a);
        if (ratio < kInf && ratio <= relaxed && std::abs(a) > leave_abs) {
          leave_abs = std::abs(a);
          leave = k;
          t_step = ratio;
        }
      }
      if (opt_.verbose)
        std::fprintf(stderr, "primal it %lld: q=%d (status %d d=%g) dir=%g tmax=%g leave=%d t_step=%g\n", static_cast<long long>(iterations_), q, static_cast<int>(st_[sq]), d_[sq], dir, tmax, leave, t_step);
      if (leave < 0 && std::isinf(tmax) && since_refactor_ > 0) { // never certify unboundedness from a drifted factorisation
        refactor();
        compute_primal();
        compute_dual();
        continue;
      }
      if (leave < 0 && std::isinf(tmax)) {
        // unbounded ray: dir on q, -dir*alpha on the basics
        out.ray.assign(static_cast<std::size_t>(n_), 0.0);
        if (q < n_)
          out.ray[static_cast<std::size_t>(q)] = dir;
        for (std::int32_t k = 0; k < m_; ++k) {
          const auto j = basic_[static_cast<std::size_t>(k)];
          if (j < n_)
            out.ray[static_cast<std::size_t>(j)] = -dir * alpha_q[static_cast<std::size_t>(k)];
        }
        for (std::int32_t j = 0; j < n_; ++j)
          out.ray[static_cast<std::size_t>(j)] *= col_scale_[static_cast<std::size_t>(j)];
        return SolveStatus::Unbounded;
      }
      if (leave < 0 || tmax <= t_step) { // bound flip of the entering variable
        const double move = dir * tmax;
        for (std::int32_t k = 0; k < m_; ++k)
          x_[static_cast<std::size_t>(basic_[static_cast<std::size_t>(k)])] -= move * alpha_q[static_cast<std::size_t>(k)];
        x_[sq] += move;
        st_[sq] = st_[sq] == VarStatus::AtLower ? VarStatus::AtUpper : VarStatus::AtLower;
        x_[sq] = st_[sq] == VarStatus::AtUpper ? hi_[sq] : lo_[sq];
        ++iterations_;
        continue;
      }
      const double move = dir * t_step;
      const auto p_var = static_cast<std::size_t>(basic_[static_cast<std::size_t>(leave)]);
      for (std::int32_t k = 0; k < m_; ++k)
        x_[static_cast<std::size_t>(basic_[static_cast<std::size_t>(k)])] -= move * alpha_q[static_cast<std::size_t>(k)];
      x_[sq] += move;
      const double a_leave = dir * alpha_q[static_cast<std::size_t>(leave)];
      st_[p_var] = a_leave > 0.0 ? VarStatus::AtLower : VarStatus::AtUpper;
      x_[p_var] = a_leave > 0.0 ? lo_[p_var] : hi_[p_var];
      // ---- pivot row to update reduced costs and Devex weights
      std::fill(rho.begin(), rho.end(), 0.0);
      rho[static_cast<std::size_t>(leave)] = 1.0;
      lu_.btran(rho);
      compute_pivot_row(rho, row, touched);
      const double alpha_rq = alpha_q[static_cast<std::size_t>(leave)];
      const double theta_d = d_[sq] / alpha_rq;
      for (const auto j : touched) {
        const auto sj = static_cast<std::size_t>(j);
        if (st_[sj] == VarStatus::Basic || j == q)
          continue;
        d_[sj] -= theta_d * row[sj];
        const double r = row[sj] / alpha_rq;
        devex[sj] = std::max(devex[sj], r * r * devex[sq]);
      }
      d_[p_var] = -theta_d;
      d_[sq] = 0.0;
      devex[p_var] = std::max(devex[sq] / (alpha_rq * alpha_rq), 1.0);
      basic_[static_cast<std::size_t>(leave)] = q;
      pos_[p_var] = -1;
      pos_[sq] = leave;
      st_[sq] = VarStatus::Basic;
      lu_.update(leave, alpha_q);
      ++since_refactor_;
      ++iterations_;
    }
  }

  // remove cost perturbation / shifts and finish with primal pivots if that exposes dual infeasibilities
  SolveStatus cleanup(LpSolution &out) {
    if (!shifted_)
      return SolveStatus::Optimal;
    cost_ = orig_cost_;
    shifted_ = false;
    compute_dual();
    bool infeasible_d = false;
    for (std::int32_t j = 0; j < n_ + m_; ++j) {
      const auto sj = static_cast<std::size_t>(j);
      if (st_[sj] == VarStatus::Basic || (lo_[sj] == hi_[sj] && !std::isinf(lo_[sj])))
        continue;
      if ((st_[sj] == VarStatus::AtLower && d_[sj] < -opt_.dual_tol) || (st_[sj] == VarStatus::AtUpper && d_[sj] > opt_.dual_tol) ||
          (st_[sj] == VarStatus::Free && std::abs(d_[sj]) > opt_.dual_tol)) {
        infeasible_d = true;
        break;
      }
    }
    if (!infeasible_d)
      return SolveStatus::Optimal;
    // primal simplex from the (primal feasible) basis; if the removal also broke primal feasibility, fall back to the dual
    SolveStatus s = primal_loop(out);
    if (s == SolveStatus::Optimal) {
      // a few dual iterations guard against primal infeasibilities introduced by flips
      compute_primal();
      bool pf = true;
      for (std::int32_t k = 0; k < m_; ++k) {
        const auto j = static_cast<std::size_t>(basic_[static_cast<std::size_t>(k)]);
        if (x_[j] < lo_[j] - 10 * opt_.primal_tol || x_[j] > hi_[j] + 10 * opt_.primal_tol)
          pf = false;
      }
      if (!pf) {
        make_dual_feasible();
        compute_primal();
        s = dual_loop(out);
      }
    }
    return s;
  }

  // Final accuracy pass: re-run the primal simplex with a 100x tighter dual tolerance (variables of magnitude 1e3..1e5 make a
  // 1e-7 reduced-cost test visible in the objective). Bounded work; the basis stays primal feasible, so a failure is harmless.
  void polish(LpSolution &out) {
    const double saved_dual = opt_.dual_tol, saved_max = opt_.max_iterations;
    opt_.dual_tol = saved_dual * 1e-2;
    opt_.max_iterations = iterations_ + std::max<std::int64_t>(2000, 2 * (m_ + n_));
    cost_ = orig_cost_;
    shifted_ = false;
    LpSolution scratch;
    const auto s = primal_loop(scratch);
    opt_.dual_tol = saved_dual;
    opt_.max_iterations = static_cast<std::int64_t>(saved_max);
    if (s != SolveStatus::Optimal && s != SolveStatus::IterationLimit)
      compute_dual(); // unreachable in practice (the basis was optimal), keep duals consistent
    (void)out;
  }

  // ------------------------------------------------------------------ results
  void finish(LpSolution &out) {
    if (opt_.verbose)
      std::fprintf(stderr, "timers: refactor %.3fs (%lld) chuzr %.3fs btran %.3fs row %.3fs chuzc %.3fs ftran %.3fs update %.3fs primal+dse %.3fs; LU nnz %lld\n", tm_.refactor, static_cast<long long>(tm_.refactors), tm_.chuzr, tm_.btran, tm_.row, tm_.chuzc, tm_.ftran, tm_.update, tm_.dse, static_cast<long long>(lu_.factor_nnz()));
    if (primal_dirty_ && !need_refactor_)
      compute_primal();
    if (out.status == SolveStatus::Optimal && !need_refactor_) {
      cost_ = orig_cost_; // report duals / reduced costs for the TRUE costs
      shifted_ = false;
      compute_dual();
    }
    out.iterations = iterations_;
    out.x.assign(static_cast<std::size_t>(n_), 0.0);
    out.row_activity.assign(static_cast<std::size_t>(m_), 0.0);
    out.reduced.assign(static_cast<std::size_t>(n_), 0.0);
    out.row_dual.assign(static_cast<std::size_t>(m_), 0.0);
    for (std::int32_t j = 0; j < n_; ++j)
      out.x[static_cast<std::size_t>(j)] = x_[static_cast<std::size_t>(j)] * col_scale_[static_cast<std::size_t>(j)];
    for (std::int32_t i = 0; i < m_; ++i)
      out.row_activity[static_cast<std::size_t>(i)] = x_[static_cast<std::size_t>(n_ + i)] / row_scale_[static_cast<std::size_t>(i)];
    if (static_cast<std::int32_t>(y_.size()) == m_) {
      for (std::int32_t i = 0; i < m_; ++i)
        out.row_dual[static_cast<std::size_t>(i)] = y_[static_cast<std::size_t>(i)] * row_scale_[static_cast<std::size_t>(i)];
      for (std::int32_t j = 0; j < n_; ++j)
        out.reduced[static_cast<std::size_t>(j)] = d_[static_cast<std::size_t>(j)] / col_scale_[static_cast<std::size_t>(j)];
    }
    if (!out.farkas.empty())
      for (std::int32_t i = 0; i < m_ && i < static_cast<std::int32_t>(out.farkas.size()); ++i)
        out.farkas[static_cast<std::size_t>(i)] *= row_scale_[static_cast<std::size_t>(i)];
    double obj = offset_;
    for (std::int32_t j = 0; j < n_; ++j)
      obj += orig_cost_[static_cast<std::size_t>(j)] * x_[static_cast<std::size_t>(j)];
    out.objective = obj;
    get_basis(out.basis);
  }

  [[nodiscard]] double elapsed() const {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - start_time_).count();
  }

  struct Timers { double refactor{0}, chuzr{0}, btran{0}, row{0}, chuzc{0}, ftran{0}, update{0}, dse{0}; std::int64_t refactors{0}; } tm_;
  static double now_s() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
  double cutoff_scaled_{kInf};
  std::int64_t per_solve_limit_{0}, base_max_iterations_{0};
  bool costs_dirty_{false};
  std::int64_t default_budget_{5'000'000};
  SimplexOptions opt_;
  std::int32_t m_{0}, n_{0};
  SparseMatrix a_, at_;
  std::vector<double> cost_, orig_cost_, lo_, hi_, orig_lo_, orig_hi_, x_, d_, y_, dse_;
  std::vector<double> col_scale_, col_scale_inv_, row_scale_;
  std::vector<VarStatus> st_;
  std::vector<std::uint32_t> mark_;
  std::uint32_t stamp_{0};
  std::vector<std::int32_t> basic_, pos_;
  BasisFactor lu_;
  double offset_{0.0};
  bool have_basis_{false}, need_refactor_{true}, primal_dirty_{true}, shifted_{false};
  int since_refactor_{0};
  std::int64_t iterations_{0};
  std::chrono::steady_clock::time_point start_time_{std::chrono::steady_clock::now()};
};

} // namespace pluss::subsp::math::lp
