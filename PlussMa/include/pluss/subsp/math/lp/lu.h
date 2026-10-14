#pragma once

// Sparse LU factorisation of a simplex basis with product-form (eta) updates.
//
// Gilbert-Peierls left-looking LU with threshold partial pivoting (relative threshold u, ties broken towards sparse rows) after a
// static column pre-ordering by column count (singleton / near-triangular columns first, which keeps simplex bases almost
// triangular and fill tiny). Each column is processed by a sparse triangular solve whose non-zero pattern comes from a depth-first
// reach on the graph of L, so the cost is proportional to the arithmetic performed.
//
//   B = columns of the basis in BASIS-POSITION order; rows are constraint rows.
//   ftran:  B x = b   (b row-indexed in, x position-indexed out, in place)
//   btran:  B^T y = c (c position-indexed in, y row-indexed out, in place)
//   update: replace the column at position r by a new column with ftran image alpha (product-form eta)
// A numerically singular basis is reported as a list of (position, row) pairs: swap the column at `position` for the logical of
// `row` and factorise again.

#include "pluss/subsp/math/lp/sparse.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>
#include <vector>

namespace pluss::subsp::math::lp {

class BasisFactor final {
public:
  struct Singular final {
    std::int32_t position;
    std::int32_t row;
  };

  // Factorise the m x m matrix with CSC arrays (start, index, value); column k is basis position k.
  // Returns the singular (position, row) repair list (empty when the basis is numerically nonsingular).
  std::vector<Singular> factorize(std::int32_t m, const std::vector<std::int64_t> &start, const std::vector<std::int32_t> &index,
                                  const std::vector<double> &value, double threshold = 0.1, double pivot_tol = 1e-9) {
    m_ = m;
    const auto M = static_cast<std::size_t>(m);
    prow_.assign(M, -1);
    qpos_.assign(M, -1);
    pinv_.assign(M, -1);
    udiag_.assign(M, 0.0);
    lstart_.assign(1, 0);
    lindex_.clear();
    lvalue_.clear();
    ustart_.assign(1, 0);
    uindex_.clear();
    uvalue_.clear();
    etas_.clear();
    eta_nnz_ = 0;
    std::vector<Singular> singular;

    std::vector<std::int32_t> order(M);
    for (std::size_t k = 0; k < M; ++k)
      order[k] = static_cast<std::int32_t>(k);
    std::stable_sort(order.begin(), order.end(), [&](std::int32_t a, std::int32_t b) {
      return start[static_cast<std::size_t>(a) + 1] - start[static_cast<std::size_t>(a)] <
             start[static_cast<std::size_t>(b) + 1] - start[static_cast<std::size_t>(b)];
    });
    std::vector<std::int32_t> rowcount(M, 0);
    for (const auto i : index)
      ++rowcount[static_cast<std::size_t>(i)];

    std::vector<double> x(M, 0.0);
    std::vector<std::int32_t> stamp(M, -1), stack, pstack, reach;
    std::vector<std::int32_t> skipped_positions;
    std::int32_t step = 0;
    std::int32_t token = 0;
    stack.reserve(M);
    pstack.reserve(M);
    for (std::size_t oi = 0; oi < M; ++oi) {
      const std::int32_t col = order[oi];
      ++token;
      reach.clear();
      // DFS reach from the column's rows through L (a pivotal row expands to the rows of its L column); post-order output
      for (std::int64_t p = start[static_cast<std::size_t>(col)]; p < start[static_cast<std::size_t>(col) + 1]; ++p) {
        const std::int32_t root = index[static_cast<std::size_t>(p)];
        if (stamp[static_cast<std::size_t>(root)] == token)
          continue;
        stack.assign(1, root);
        pstack.assign(1, 0);
        stamp[static_cast<std::size_t>(root)] = token;
        while (!stack.empty()) {
          const std::int32_t node = stack.back();
          const std::int32_t pv = pinv_[static_cast<std::size_t>(node)];
          std::int64_t pbeg = 0, pend = 0;
          if (pv >= 0) {
            pbeg = lstart_[static_cast<std::size_t>(pv)];
            pend = lstart_[static_cast<std::size_t>(pv) + 1];
          }
          bool descended = false;
          for (std::int64_t q = pbeg + pstack.back(); q < pend; ++q) {
            const std::int32_t child = lindex_[static_cast<std::size_t>(q)];
            if (stamp[static_cast<std::size_t>(child)] == token)
              continue;
            stamp[static_cast<std::size_t>(child)] = token;
            pstack.back() = static_cast<std::int32_t>(q - pbeg + 1);
            stack.push_back(child);
            pstack.push_back(0);
            descended = true;
            break;
          }
          if (!descended) {
            reach.push_back(node);
            stack.pop_back();
            pstack.pop_back();
          }
        }
      }
      for (std::int64_t p = start[static_cast<std::size_t>(col)]; p < start[static_cast<std::size_t>(col) + 1]; ++p)
        x[static_cast<std::size_t>(index[static_cast<std::size_t>(p)])] = value[static_cast<std::size_t>(p)];
      for (std::size_t r = reach.size(); r-- > 0;) { // reverse post-order = topological order
        const std::int32_t node = reach[r];
        const std::int32_t pv = pinv_[static_cast<std::size_t>(node)];
        if (pv < 0)
          continue;
        const double t = x[static_cast<std::size_t>(node)];
        if (t == 0.0)
          continue;
        for (std::int64_t q = lstart_[static_cast<std::size_t>(pv)]; q < lstart_[static_cast<std::size_t>(pv) + 1]; ++q)
          x[static_cast<std::size_t>(lindex_[static_cast<std::size_t>(q)])] -= lvalue_[static_cast<std::size_t>(q)] * t;
      }
      double maxabs = 0.0;
      for (const auto i : reach)
        if (pinv_[static_cast<std::size_t>(i)] < 0)
          maxabs = std::max(maxabs, std::abs(x[static_cast<std::size_t>(i)]));
      std::int32_t pivot_row = -1;
      if (maxabs > pivot_tol) {
        double best_abs = 0.0;
        std::int32_t best_count = std::numeric_limits<std::int32_t>::max();
        for (const auto i : reach) {
          if (pinv_[static_cast<std::size_t>(i)] >= 0)
            continue;
          const double a = std::abs(x[static_cast<std::size_t>(i)]);
          if (a < threshold * maxabs)
            continue;
          const std::int32_t rc = rowcount[static_cast<std::size_t>(i)];
          if (rc < best_count || (rc == best_count && a > best_abs)) {
            best_count = rc;
            best_abs = a;
            pivot_row = i;
          }
        }
      }
      if (pivot_row < 0) {
        for (const auto i : reach)
          x[static_cast<std::size_t>(i)] = 0.0;
        skipped_positions.push_back(col);
        continue;
      }
      const double pivot = x[static_cast<std::size_t>(pivot_row)];
      for (const auto i : reach) {
        const std::int32_t pv = pinv_[static_cast<std::size_t>(i)];
        const double v = x[static_cast<std::size_t>(i)];
        if (pv >= 0) {
          if (v != 0.0) {
            uindex_.push_back(pv);
            uvalue_.push_back(v);
          }
        } else if (i != pivot_row && v != 0.0) {
          lindex_.push_back(i);
          lvalue_.push_back(v / pivot);
        }
      }
      ustart_.push_back(static_cast<std::int64_t>(uindex_.size()));
      lstart_.push_back(static_cast<std::int64_t>(lindex_.size()));
      udiag_[static_cast<std::size_t>(step)] = pivot;
      prow_[static_cast<std::size_t>(step)] = pivot_row;
      pinv_[static_cast<std::size_t>(pivot_row)] = step;
      qpos_[static_cast<std::size_t>(step)] = col;
      for (const auto i : reach)
        x[static_cast<std::size_t>(i)] = 0.0;
      ++step;
    }
    steps_ = step;
    qinv_.clear();
    build_transposes_();
    if (!skipped_positions.empty()) {
      std::vector<std::int32_t> free_rows;
      for (std::int32_t i = 0; i < m; ++i)
        if (pinv_[static_cast<std::size_t>(i)] < 0)
          free_rows.push_back(i);
      for (std::size_t k = 0; k < skipped_positions.size() && k < free_rows.size(); ++k)
        singular.push_back({skipped_positions[k], free_rows[k]});
    }
    return singular;
  }

  [[nodiscard]] std::int32_t dimension() const { return m_; }
  [[nodiscard]] std::size_t eta_count() const { return etas_.size(); }
  [[nodiscard]] std::int64_t factor_nnz() const { return static_cast<std::int64_t>(lindex_.size() + uindex_.size()) + m_; }
  [[nodiscard]] std::int64_t eta_nnz() const { return eta_nnz_; }

  // `nz` (optional) receives a superset of the non-zero positions of the result, so callers can update their own vectors in O(nnz).
  // `hint` (optional) lists the rows that may be non-zero in `rhs`; it saves the scan that finds them.
  void ftran(std::vector<double> &rhs, std::vector<std::int32_t> *nz = nullptr, const std::vector<std::int32_t> *hint = nullptr) const {
    // hypersparse path: when the right-hand side has few non-zeros, the L and U solves visit only the steps reachable from them
    // (Gilbert & Peierls 1988); otherwise the dense sweep is faster.
    if (m_ >= 1500 && hint && hint->size() * 10 < static_cast<std::size_t>(m_)) {
      ftran_sparse_(rhs, nz, hint);
      return;
    }
    if (m_ >= 1500) {
      std::size_t nnz = 0;
      for (const double v : rhs)
        nnz += v != 0.0;
      if (nnz * 10 < static_cast<std::size_t>(m_)) {
        ftran_sparse_(rhs, nz);
        return;
      }
    }
    ftran_dense_(rhs);
    if (nz) {
      nz->clear();
      for (std::size_t i = 0; i < static_cast<std::size_t>(m_); ++i)
        if (rhs[i] != 0.0)
          nz->push_back(static_cast<std::int32_t>(i));
    }
  }

  // Solve B x = b. `rhs` is row-indexed on entry and basis-position-indexed on exit.
  void ftran_dense_(std::vector<double> &rhs) const {
    const auto M = static_cast<std::size_t>(m_);
    for (std::int32_t k = 0; k < steps_; ++k) {
      const double t = rhs[static_cast<std::size_t>(prow_[static_cast<std::size_t>(k)])];
      if (t == 0.0)
        continue;
      for (std::int64_t q = lstart_[static_cast<std::size_t>(k)]; q < lstart_[static_cast<std::size_t>(k) + 1]; ++q)
        rhs[static_cast<std::size_t>(lindex_[static_cast<std::size_t>(q)])] -= lvalue_[static_cast<std::size_t>(q)] * t;
    }
    work_.assign(M, 0.0);
    for (std::int32_t k = 0; k < steps_; ++k)
      work_[static_cast<std::size_t>(k)] = rhs[static_cast<std::size_t>(prow_[static_cast<std::size_t>(k)])];
    for (std::int32_t k = steps_; k-- > 0;) {
      double y = work_[static_cast<std::size_t>(k)];
      if (y == 0.0)
        continue;
      y /= udiag_[static_cast<std::size_t>(k)];
      work_[static_cast<std::size_t>(k)] = y;
      for (std::int64_t q = ustart_[static_cast<std::size_t>(k)]; q < ustart_[static_cast<std::size_t>(k) + 1]; ++q)
        work_[static_cast<std::size_t>(uindex_[static_cast<std::size_t>(q)])] -= uvalue_[static_cast<std::size_t>(q)] * y;
    }
    std::fill(rhs.begin(), rhs.end(), 0.0);
    for (std::int32_t k = 0; k < steps_; ++k)
      rhs[static_cast<std::size_t>(qpos_[static_cast<std::size_t>(k)])] = work_[static_cast<std::size_t>(k)];
    for (const auto &e : etas_) {
      const double xr = rhs[static_cast<std::size_t>(e.r)] / e.pivot;
      rhs[static_cast<std::size_t>(e.r)] = xr;
      if (xr != 0.0)
        for (std::size_t q = 0; q < e.idx.size(); ++q)
          rhs[static_cast<std::size_t>(e.idx[q])] -= e.val[q] * xr;
    }
  }

  // Solve B^T y = c. `c` is basis-position-indexed on entry and row-indexed on exit.
  // Scatter-form triangular solves over row-wise copies of U and L: work is proportional to the entries actually touched (a zero
  // value is skipped without reading its row), the sparse-right-hand-side behaviour that makes BTRAN cheap on large sparse bases.
  void btran(std::vector<double> &c) const {
    if (m_ >= 1500) {
      std::size_t nnz = 0;
      for (const double v : c)
        nnz += v != 0.0;
      if (nnz * 10 < static_cast<std::size_t>(m_)) {
        btran_sparse_(c);
        return;
      }
    }
    btran_dense_(c);
  }

  void btran_dense_(std::vector<double> &c) const {
    const auto M = static_cast<std::size_t>(m_);
    for (std::size_t t = etas_.size(); t-- > 0;) {
      const auto &e = etas_[t];
      double s = c[static_cast<std::size_t>(e.r)];
      for (std::size_t q = 0; q < e.idx.size(); ++q)
        s -= e.val[q] * c[static_cast<std::size_t>(e.idx[q])];
      c[static_cast<std::size_t>(e.r)] = s / e.pivot;
    }
    work_.assign(M, 0.0);
    for (std::int32_t k = 0; k < steps_; ++k)
      work_[static_cast<std::size_t>(k)] = c[static_cast<std::size_t>(qpos_[static_cast<std::size_t>(k)])];
    // U^T solve: y_k = acc_k / u_kk, then push y_k into every later step whose U column holds an entry in row k
    for (std::int32_t k = 0; k < steps_; ++k) {
      double y = work_[static_cast<std::size_t>(k)];
      if (y == 0.0)
        continue;
      y /= udiag_[static_cast<std::size_t>(k)];
      work_[static_cast<std::size_t>(k)] = y;
      for (std::int64_t q = utstart_[static_cast<std::size_t>(k)]; q < utstart_[static_cast<std::size_t>(k) + 1]; ++q)
        work_[static_cast<std::size_t>(utindex_[static_cast<std::size_t>(q)])] -= utvalue_[static_cast<std::size_t>(q)] * y;
    }
    std::fill(c.begin(), c.end(), 0.0);
    for (std::int32_t k = 0; k < steps_; ++k)
      c[static_cast<std::size_t>(prow_[static_cast<std::size_t>(k)])] = work_[static_cast<std::size_t>(k)];
    // L^T solve (descending): the value of row i (step j) is final once all later steps have pushed; push it to earlier columns
    for (std::int32_t j = steps_; j-- > 0;) {
      const auto row = static_cast<std::size_t>(prow_[static_cast<std::size_t>(j)]);
      const double v = c[row];
      if (v == 0.0)
        continue;
      for (std::int64_t q = ltstart_[row]; q < ltstart_[row + 1]; ++q)
        c[static_cast<std::size_t>(prow_[static_cast<std::size_t>(ltindex_[static_cast<std::size_t>(q)])])] -= ltvalue_[static_cast<std::size_t>(q)] * v;
    }
  }

  // Replace the column at position r by a column whose ftran image is `alpha` (dense, position-indexed).
  void update(std::int32_t r, const std::vector<double> &alpha, const std::vector<std::int32_t> *nz = nullptr) {
    Eta e;
    e.r = r;
    e.pivot = alpha[static_cast<std::size_t>(r)];
    if (nz) {
      for (const auto i : *nz)
        if (i != r && std::abs(alpha[static_cast<std::size_t>(i)]) > 1e-14) {
          e.idx.push_back(i);
          e.val.push_back(alpha[static_cast<std::size_t>(i)]);
        }
    } else {
      for (std::int32_t i = 0; i < m_; ++i)
        if (i != r && std::abs(alpha[static_cast<std::size_t>(i)]) > 1e-14) {
          e.idx.push_back(i);
          e.val.push_back(alpha[static_cast<std::size_t>(i)]);
        }
    }
    eta_nnz_ += static_cast<std::int64_t>(e.idx.size()) + 1;
    etas_.push_back(std::move(e));
  }

private:

  // iterative DFS reach over a step graph; appends the reached steps to `topo` in topological (reverse postorder) order
  template <class EdgeFn> void reach_(const std::vector<std::int32_t> &seeds, EdgeFn &&edges, std::vector<std::int32_t> &topo) const {
    if (mark_.size() != static_cast<std::size_t>(m_)) {
      mark_.assign(static_cast<std::size_t>(m_), 0);
      stamp_ = 0;
    }
    ++stamp_;
    topo.clear();
    std::vector<std::pair<std::int32_t, std::int64_t>> stack;
    for (const auto seed : seeds) {
      if (mark_[static_cast<std::size_t>(seed)] == stamp_)
        continue;
      mark_[static_cast<std::size_t>(seed)] = stamp_;
      stack.push_back({seed, -1});
      while (!stack.empty()) {
        auto &[node, pos] = stack.back();
        std::int64_t begin, end;
        edges.range(node, begin, end);
        if (pos < 0)
          pos = begin;
        bool pushed = false;
        while (pos < end) {
          const auto nxt = edges.target(pos++);
          if (nxt >= 0 && mark_[static_cast<std::size_t>(nxt)] != stamp_) {
            mark_[static_cast<std::size_t>(nxt)] = stamp_;
            stack.push_back({nxt, -1});
            pushed = true;
            break;
          }
        }
        if (!pushed) {
          topo.push_back(node);
          stack.pop_back();
        }
      }
    }
    std::reverse(topo.begin(), topo.end());
  }

  struct LEdges final {
    const BasisFactor *f;
    void range(std::int32_t k, std::int64_t &b, std::int64_t &e) const { b = f->lstart_[static_cast<std::size_t>(k)]; e = f->lstart_[static_cast<std::size_t>(k) + 1]; }
    [[nodiscard]] std::int32_t target(std::int64_t q) const { return f->pinv_[static_cast<std::size_t>(f->lindex_[static_cast<std::size_t>(q)])]; }
  };
  struct UEdges final {
    const BasisFactor *f;
    void range(std::int32_t k, std::int64_t &b, std::int64_t &e) const { b = f->ustart_[static_cast<std::size_t>(k)]; e = f->ustart_[static_cast<std::size_t>(k) + 1]; }
    [[nodiscard]] std::int32_t target(std::int64_t q) const { return f->uindex_[static_cast<std::size_t>(q)]; }
  };


  struct UtEdges final {
    const BasisFactor *f;
    void range(std::int32_t k, std::int64_t &b, std::int64_t &e) const { b = f->utstart_[static_cast<std::size_t>(k)]; e = f->utstart_[static_cast<std::size_t>(k) + 1]; }
    [[nodiscard]] std::int32_t target(std::int64_t q) const { return f->utindex_[static_cast<std::size_t>(q)]; }
  };
  struct LtEdges final {
    const BasisFactor *f;
    void range(std::int32_t j, std::int64_t &b, std::int64_t &e) const {
      const auto row = static_cast<std::size_t>(f->prow_[static_cast<std::size_t>(j)]);
      b = f->ltstart_[row];
      e = f->ltstart_[row + 1];
    }
    [[nodiscard]] std::int32_t target(std::int64_t q) const { return f->ltindex_[static_cast<std::size_t>(q)]; }
  };

  void btran_sparse_(std::vector<double> &c) const {
    const auto M = static_cast<std::size_t>(m_);
    for (std::size_t t = etas_.size(); t-- > 0;) {
      const auto &e = etas_[t];
      double sum = c[static_cast<std::size_t>(e.r)];
      for (std::size_t q = 0; q < e.idx.size(); ++q)
        sum -= e.val[q] * c[static_cast<std::size_t>(e.idx[q])];
      c[static_cast<std::size_t>(e.r)] = sum / e.pivot;
    }
    if (qinv_.size() != M) {
      qinv_.assign(M, -1);
      for (std::int32_t k = 0; k < steps_; ++k)
        qinv_[static_cast<std::size_t>(qpos_[static_cast<std::size_t>(k)])] = k;
    }
    seeds_.clear();
    for (std::size_t pos = 0; pos < M; ++pos)
      if (c[pos] != 0.0) {
        if (qinv_[pos] < 0) { // position without a pivot (singular repair pending): fall back
          btran_dense_(c);
          return;
        }
        seeds_.push_back(qinv_[pos]);
      }
    if (seeds_.empty())
      return;
    if (swork_.size() != M)
      swork_.assign(M, 0.0); // invariant: all zero between calls
    for (const auto k : seeds_) {
      const auto pos = static_cast<std::size_t>(qpos_[static_cast<std::size_t>(k)]);
      swork_[static_cast<std::size_t>(k)] = c[pos];
      c[pos] = 0.0;
    }
    reach_(seeds_, UtEdges{this}, topo_u_);
    for (const auto k : topo_u_) {
      double y = swork_[static_cast<std::size_t>(k)];
      if (y == 0.0)
        continue;
      y /= udiag_[static_cast<std::size_t>(k)];
      swork_[static_cast<std::size_t>(k)] = y;
      for (std::int64_t q = utstart_[static_cast<std::size_t>(k)]; q < utstart_[static_cast<std::size_t>(k) + 1]; ++q)
        swork_[static_cast<std::size_t>(utindex_[static_cast<std::size_t>(q)])] -= utvalue_[static_cast<std::size_t>(q)] * y;
    }
    for (const auto k : topo_u_) {
      c[static_cast<std::size_t>(prow_[static_cast<std::size_t>(k)])] = swork_[static_cast<std::size_t>(k)];
      swork_[static_cast<std::size_t>(k)] = 0.0;
    }
    reach_(topo_u_, LtEdges{this}, topo_l_);
    for (const auto j : topo_l_) {
      const auto row = static_cast<std::size_t>(prow_[static_cast<std::size_t>(j)]);
      const double v = c[row];
      if (v == 0.0)
        continue;
      for (std::int64_t q = ltstart_[row]; q < ltstart_[row + 1]; ++q)
        c[static_cast<std::size_t>(prow_[static_cast<std::size_t>(ltindex_[static_cast<std::size_t>(q)])])] -= ltvalue_[static_cast<std::size_t>(q)] * v;
    }
  }

  void ftran_sparse_(std::vector<double> &rhs, std::vector<std::int32_t> *nz, const std::vector<std::int32_t> *hint = nullptr) const {
    const auto M = static_cast<std::size_t>(m_);
    seeds_.clear();
    const std::size_t scan = hint ? hint->size() : M;
    for (std::size_t it = 0; it < scan; ++it) {
      const std::size_t i = hint ? static_cast<std::size_t>((*hint)[it]) : it;
      if (rhs[i] != 0.0) {
        const auto st = pinv_[i];
        if (st < 0) { // row without a pivot (singular repair pending): fall back
          ftran_dense_(rhs);
          if (nz) {
            nz->clear();
            for (std::size_t q = 0; q < M; ++q)
              if (rhs[q] != 0.0)
                nz->push_back(static_cast<std::int32_t>(q));
          }
          return;
        }
        seeds_.push_back(st);
      }
    }
    if (seeds_.empty()) {
      if (nz)
        nz->clear();
      return;
    }
    reach_(seeds_, LEdges{this}, topo_l_);
    for (const auto k : topo_l_) {
      const double t = rhs[static_cast<std::size_t>(prow_[static_cast<std::size_t>(k)])];
      if (t == 0.0)
        continue;
      for (std::int64_t q = lstart_[static_cast<std::size_t>(k)]; q < lstart_[static_cast<std::size_t>(k) + 1]; ++q)
        rhs[static_cast<std::size_t>(lindex_[static_cast<std::size_t>(q)])] -= lvalue_[static_cast<std::size_t>(q)] * t;
    }
    if (swork_.size() != M)
      swork_.assign(M, 0.0); // invariant: all zero between calls
    for (const auto k : topo_l_) {
      const auto row = static_cast<std::size_t>(prow_[static_cast<std::size_t>(k)]);
      swork_[static_cast<std::size_t>(k)] = rhs[row];
      rhs[row] = 0.0;
    }
    reach_(topo_l_, UEdges{this}, topo_u_);
    for (const auto k : topo_u_) { // topological order of the U graph (edges point to smaller steps): process largest dependencies first
      double y = swork_[static_cast<std::size_t>(k)];
      if (y == 0.0)
        continue;
      y /= udiag_[static_cast<std::size_t>(k)];
      swork_[static_cast<std::size_t>(k)] = y;
      for (std::int64_t q = ustart_[static_cast<std::size_t>(k)]; q < ustart_[static_cast<std::size_t>(k) + 1]; ++q)
        swork_[static_cast<std::size_t>(uindex_[static_cast<std::size_t>(q)])] -= uvalue_[static_cast<std::size_t>(q)] * y;
    }
    if (pmark_.size() != M) {
      pmark_.assign(M, 0);
      pstamp_ = 0;
    }
    ++pstamp_;
    if (nz)
      nz->clear();
    const auto note = [&](std::int32_t pos) {
      if (nz && pmark_[static_cast<std::size_t>(pos)] != pstamp_) {
        pmark_[static_cast<std::size_t>(pos)] = pstamp_;
        nz->push_back(pos);
      }
    };
    for (const auto k : topo_u_) {
      const auto pos = qpos_[static_cast<std::size_t>(k)];
      rhs[static_cast<std::size_t>(pos)] = swork_[static_cast<std::size_t>(k)];
      swork_[static_cast<std::size_t>(k)] = 0.0;
      note(pos);
    }
    for (const auto &e : etas_) {
      const double xr = rhs[static_cast<std::size_t>(e.r)];
      if (xr == 0.0)
        continue;
      const double v = xr / e.pivot;
      rhs[static_cast<std::size_t>(e.r)] = v;
      for (std::size_t q = 0; q < e.idx.size(); ++q) {
        rhs[static_cast<std::size_t>(e.idx[q])] -= e.val[q] * v;
        note(e.idx[q]);
      }
    }
  }
  // row-wise copies: U^T by step (entries (later step, value)); L^T by row (entries (earlier step whose L column holds the row, value))
  void build_transposes_() {
    const auto S = static_cast<std::size_t>(steps_), M = static_cast<std::size_t>(m_);
    utstart_.assign(S + 1, 0);
    for (std::size_t q = 0; q < uindex_.size(); ++q)
      ++utstart_[static_cast<std::size_t>(uindex_[q]) + 1];
    for (std::size_t k = 0; k < S; ++k)
      utstart_[k + 1] += utstart_[k];
    utindex_.assign(uindex_.size(), 0);
    utvalue_.assign(uindex_.size(), 0.0);
    {
      std::vector<std::int64_t> pos(utstart_.begin(), utstart_.end() - 1);
      for (std::size_t k = 0; k < S; ++k)
        for (std::int64_t q = ustart_[k]; q < ustart_[k + 1]; ++q) {
          const auto at = static_cast<std::size_t>(pos[static_cast<std::size_t>(uindex_[static_cast<std::size_t>(q)])]++);
          utindex_[at] = static_cast<std::int32_t>(k);
          utvalue_[at] = uvalue_[static_cast<std::size_t>(q)];
        }
    }
    ltstart_.assign(M + 1, 0);
    for (std::size_t q = 0; q < lindex_.size(); ++q)
      ++ltstart_[static_cast<std::size_t>(lindex_[q]) + 1];
    for (std::size_t i = 0; i < M; ++i)
      ltstart_[i + 1] += ltstart_[i];
    ltindex_.assign(lindex_.size(), 0);
    ltvalue_.assign(lindex_.size(), 0.0);
    std::vector<std::int64_t> pos(ltstart_.begin(), ltstart_.end() - 1);
    for (std::size_t k = 0; k < S; ++k)
      for (std::int64_t q = lstart_[k]; q < lstart_[k + 1]; ++q) {
        const auto at = static_cast<std::size_t>(pos[static_cast<std::size_t>(lindex_[static_cast<std::size_t>(q)])]++);
        ltindex_[at] = static_cast<std::int32_t>(k);
        ltvalue_[at] = lvalue_[static_cast<std::size_t>(q)];
      }
  }
  std::vector<std::int64_t> utstart_, ltstart_;
  std::vector<std::int32_t> utindex_, ltindex_;
  std::vector<double> utvalue_, ltvalue_;
  struct Eta final {
    std::int32_t r{0};
    double pivot{1.0};
    std::vector<std::int32_t> idx;
    std::vector<double> val;
  };
  std::int32_t m_{0}, steps_{0};
  std::vector<std::int32_t> prow_, qpos_, pinv_;
  std::vector<double> udiag_;
  std::vector<std::int64_t> lstart_{0}, ustart_{0};
  std::vector<std::int32_t> lindex_, uindex_;
  std::vector<double> lvalue_, uvalue_;
  std::vector<Eta> etas_;
  std::int64_t eta_nnz_{0};
  mutable std::vector<double> work_, swork_;
  mutable std::vector<std::int32_t> qinv_;
  mutable std::vector<std::int32_t> seeds_, topo_l_, topo_u_, mark_;
  mutable std::int32_t stamp_{0};
  mutable std::vector<std::int32_t> pmark_;
  mutable std::int32_t pstamp_{0};
};

} // namespace pluss::subsp::math::lp
