#pragma once

// Sparse Cholesky for symmetric positive definite systems (normal equations of interior-point methods).
//   ordering : minimum degree on the quotient graph with element absorption and an approximate (upper-bound) external degree
//              (Amestoy-Davis-Duff style bound without aggressive absorption)
//   symbolic : elimination tree (Liu) + column counts from row subtrees
//   numeric  : up-looking factorisation (Davis, "Direct Methods for Sparse Linear Systems", ch. 4), with a dynamic diagonal safeguard:
//              pivots below `min_pivot` are replaced by a large value so that near-singular directions are effectively frozen
//   solve    : permuted forward / backward substitution.
// Input: the FULL symmetric matrix in CSC (both triangles); the factor is of P A P^T = L L^T.

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numeric>
#include <set>
#include <stdexcept>
#include <vector>

namespace pluss::subsp::math::lp {

class SparseCholesky final {
public:
  struct Stats final {
    std::int64_t factor_nnz{0};
    double flops{0.0};
    int regularised_pivots{0};
  };

  // Analyse the pattern of the symmetric matrix (colptr / rowidx of both triangles).
  void analyze(std::int32_t n, const std::vector<std::int64_t> &colptr, const std::vector<std::int32_t> &rowidx) {
    n_ = n;
    order_ = minimum_degree(n, colptr, rowidx);
    inv_.assign(static_cast<std::size_t>(n), 0);
    for (std::int32_t k = 0; k < n; ++k)
      inv_[static_cast<std::size_t>(order_[static_cast<std::size_t>(k)])] = k;
    // permuted upper-triangular pattern: for each column k of PAP^T, rows i <= k
    std::vector<std::vector<std::int32_t>> up(static_cast<std::size_t>(n));
    for (std::int32_t j = 0; j < n; ++j) {
      const auto pj = inv_[static_cast<std::size_t>(j)];
      for (std::int64_t p = colptr[static_cast<std::size_t>(j)]; p < colptr[static_cast<std::size_t>(j) + 1]; ++p) {
        const auto pi = inv_[static_cast<std::size_t>(rowidx[static_cast<std::size_t>(p)])];
        if (pi <= pj)
          up[static_cast<std::size_t>(pj)].push_back(pi);
      }
    }
    upstart_.assign(1, 0);
    upidx_.clear();
    for (auto &c : up) {
      std::sort(c.begin(), c.end());
      c.erase(std::unique(c.begin(), c.end()), c.end());
      upidx_.insert(upidx_.end(), c.begin(), c.end());
      upstart_.push_back(static_cast<std::int64_t>(upidx_.size()));
    }
    // elimination tree
    parent_.assign(static_cast<std::size_t>(n), -1);
    std::vector<std::int32_t> ancestor(static_cast<std::size_t>(n), -1);
    for (std::int32_t k = 0; k < n; ++k)
      for (std::int64_t p = upstart_[static_cast<std::size_t>(k)]; p < upstart_[static_cast<std::size_t>(k) + 1]; ++p) {
        std::int32_t i = upidx_[static_cast<std::size_t>(p)];
        while (i != -1 && i < k) {
          const std::int32_t next = ancestor[static_cast<std::size_t>(i)];
          ancestor[static_cast<std::size_t>(i)] = k;
          if (next == -1)
            parent_[static_cast<std::size_t>(i)] = k;
          i = next;
        }
      }
    // column counts: row k of L has the pattern of the union of tree paths from the nonzeros of column k of A (ereach)
    std::vector<std::int32_t> count(static_cast<std::size_t>(n), 0), mark(static_cast<std::size_t>(n), -1);
    for (std::int32_t k = 0; k < n; ++k) {
      mark[static_cast<std::size_t>(k)] = k;
      for (std::int64_t p = upstart_[static_cast<std::size_t>(k)]; p < upstart_[static_cast<std::size_t>(k) + 1]; ++p) {
        std::int32_t i = upidx_[static_cast<std::size_t>(p)];
        while (i < k && mark[static_cast<std::size_t>(i)] != k) {
          ++count[static_cast<std::size_t>(i)];
          mark[static_cast<std::size_t>(i)] = k;
          i = parent_[static_cast<std::size_t>(i)];
        }
      }
    }
    lstart_.assign(static_cast<std::size_t>(n) + 1, 0);
    for (std::int32_t k = 0; k < n; ++k)
      lstart_[static_cast<std::size_t>(k) + 1] = lstart_[static_cast<std::size_t>(k)] + count[static_cast<std::size_t>(k)];
    lidx_.assign(static_cast<std::size_t>(lstart_.back()), 0);
    lval_.assign(static_cast<std::size_t>(lstart_.back()), 0.0);
    stats_.factor_nnz = lstart_.back();
    double fl = 0.0;
    for (const auto c : count)
      fl += static_cast<double>(c) * static_cast<double>(c);
    stats_.flops = fl;
    analysed_ = true;
  }

  // Numeric factorisation of the matrix with the same pattern analysed earlier; `values` parallels rowidx of both triangles.
  // Returns false only if the matrix is badly non-positive-definite even after safeguards.
  bool factor(const std::vector<std::int64_t> &colptr, const std::vector<std::int32_t> &rowidx, const std::vector<double> &values,
              double min_pivot = 1e-13, double rel_pivot = 1e-14) {
    if (!analysed_)
      throw std::logic_error("SparseCholesky: analyze() first");
    const auto n = static_cast<std::size_t>(n_);
    // permuted upper values
    std::vector<std::vector<std::pair<std::int32_t, double>>> up(n);
    for (std::int32_t j = 0; j < n_; ++j) {
      const auto pj = inv_[static_cast<std::size_t>(j)];
      for (std::int64_t p = colptr[static_cast<std::size_t>(j)]; p < colptr[static_cast<std::size_t>(j) + 1]; ++p) {
        const auto pi = inv_[static_cast<std::size_t>(rowidx[static_cast<std::size_t>(p)])];
        if (pi <= pj)
          up[static_cast<std::size_t>(pj)].push_back({pi, values[static_cast<std::size_t>(p)]});
      }
    }
    std::vector<std::int32_t> fill(n, 0), stack(n), mark(n, -1);
    std::vector<double> x(n, 0.0);
    diag_.assign(n, 0.0);
    stats_.regularised_pivots = 0;
    double max_diag = 0.0;
    for (std::size_t c = 0; c < n; ++c)
      for (const auto &e : up[c])
        if (e.first == static_cast<std::int32_t>(c))
          max_diag = std::max(max_diag, std::abs(e.second));
    for (std::int32_t k = 0; k < n_; ++k) {
      const auto sk = static_cast<std::size_t>(k);
      // pattern of row k via etree reach; scatter A(0..k, k)
      std::int32_t top = n_;
      mark[sk] = k;
      double d = 0.0;
      for (const auto &e : up[sk]) {
        if (e.first == k) {
          d += e.second;
          continue;
        }
        x[static_cast<std::size_t>(e.first)] += e.second;
        std::int32_t i = e.first, len = 0;
        while (mark[static_cast<std::size_t>(i)] != k) {
          stack[static_cast<std::size_t>(len++)] = i;
          mark[static_cast<std::size_t>(i)] = k;
          i = parent_[static_cast<std::size_t>(i)];
        }
        while (len > 0)
          stack[static_cast<std::size_t>(--top)] = stack[static_cast<std::size_t>(--len)];
      }
      for (; top < n_; ++top) {
        const auto i = static_cast<std::size_t>(stack[static_cast<std::size_t>(top)]);
        const double yi = x[i];
        x[i] = 0.0;
        for (std::int64_t p = lstart_[i]; p < lstart_[i] + fill[i]; ++p)
          x[static_cast<std::size_t>(lidx_[static_cast<std::size_t>(p)])] -= lval_[static_cast<std::size_t>(p)] * yi;
        const double lki = yi / diag_[i];
        d -= lki * yi;
        const std::int64_t q = lstart_[i] + fill[i]++;
        lidx_[static_cast<std::size_t>(q)] = k;
        lval_[static_cast<std::size_t>(q)] = lki;
      }
      // LDL^T: diag_ holds D; guard tiny / negative pivots
      const double scale = std::max(1.0, max_diag);
      if (!(d > min_pivot * scale && d > rel_pivot * std::abs(d + 1.0))) {
        d = 1e128; // freeze the direction (effectively infinite pivot)
        ++stats_.regularised_pivots;
      }
      diag_[sk] = d;
    }
    return true;
  }

  // solve A x = b (b overwritten by x)
  void solve(std::vector<double> &b) const {
    const auto n = static_cast<std::size_t>(n_);
    std::vector<double> y(n);
    for (std::size_t k = 0; k < n; ++k)
      y[k] = b[static_cast<std::size_t>(order_[k])];
    // L stored by columns (strict lower, unit diagonal), D separate: forward L y = b
    for (std::size_t j = 0; j < n; ++j) {
      const double yj = y[j];
      if (yj == 0.0)
        continue;
      for (std::int64_t p = lstart_[j]; p < lstart_[j + 1]; ++p)
        y[static_cast<std::size_t>(lidx_[static_cast<std::size_t>(p)])] -= lval_[static_cast<std::size_t>(p)] * yj;
    }
    for (std::size_t j = 0; j < n; ++j)
      y[j] /= diag_[j];
    for (std::size_t j = n; j-- > 0;) {
      double s = y[j];
      for (std::int64_t p = lstart_[j]; p < lstart_[j + 1]; ++p)
        s -= lval_[static_cast<std::size_t>(p)] * y[static_cast<std::size_t>(lidx_[static_cast<std::size_t>(p)])];
      y[j] = s;
    }
    for (std::size_t k = 0; k < n; ++k)
      b[static_cast<std::size_t>(order_[k])] = y[k];
  }

  [[nodiscard]] const Stats &stats() const { return stats_; }
  [[nodiscard]] std::int32_t dimension() const { return n_; }

private:
  static std::vector<std::int32_t> minimum_degree(std::int32_t n, const std::vector<std::int64_t> &colptr, const std::vector<std::int32_t> &rowidx) {
    const auto N = static_cast<std::size_t>(n);
    std::vector<std::vector<std::int32_t>> vadj(N), eadj(N), emem(N);
    for (std::int32_t j = 0; j < n; ++j)
      for (std::int64_t p = colptr[static_cast<std::size_t>(j)]; p < colptr[static_cast<std::size_t>(j) + 1]; ++p) {
        const auto i = rowidx[static_cast<std::size_t>(p)];
        if (i != j)
          vadj[static_cast<std::size_t>(j)].push_back(i);
      }
    for (auto &a : vadj) {
      std::sort(a.begin(), a.end());
      a.erase(std::unique(a.begin(), a.end()), a.end());
    }
    std::vector<char> alive(N, 1), elem_alive(N, 0);
    std::vector<std::int32_t> degree(N), stamp(N, -1);
    std::set<std::pair<std::int32_t, std::int32_t>> queue;
    for (std::size_t i = 0; i < N; ++i) {
      degree[i] = static_cast<std::int32_t>(vadj[i].size());
      queue.insert({degree[i], static_cast<std::int32_t>(i)});
    }
    std::vector<std::int32_t> order;
    order.reserve(N);
    std::int32_t token = 0;
    while (!queue.empty()) {
      const auto it = queue.begin();
      const std::int32_t p = it->second;
      queue.erase(it);
      order.push_back(p);
      alive[static_cast<std::size_t>(p)] = 0;
      ++token;
      std::vector<std::int32_t> lp;
      stamp[static_cast<std::size_t>(p)] = token;
      for (const auto v : vadj[static_cast<std::size_t>(p)])
        if (alive[static_cast<std::size_t>(v)] && stamp[static_cast<std::size_t>(v)] != token) {
          stamp[static_cast<std::size_t>(v)] = token;
          lp.push_back(v);
        }
      for (const auto e : eadj[static_cast<std::size_t>(p)]) {
        if (!elem_alive[static_cast<std::size_t>(e)])
          continue;
        for (const auto v : emem[static_cast<std::size_t>(e)])
          if (alive[static_cast<std::size_t>(v)] && stamp[static_cast<std::size_t>(v)] != token) {
            stamp[static_cast<std::size_t>(v)] = token;
            lp.push_back(v);
          }
        elem_alive[static_cast<std::size_t>(e)] = 0; // absorbed into the new element
        emem[static_cast<std::size_t>(e)].clear();
        emem[static_cast<std::size_t>(e)].shrink_to_fit();
      }
      vadj[static_cast<std::size_t>(p)].clear();
      eadj[static_cast<std::size_t>(p)].clear();
      emem[static_cast<std::size_t>(p)] = lp;
      elem_alive[static_cast<std::size_t>(p)] = 1;
      const auto lsize = static_cast<std::int32_t>(lp.size());
      const std::int32_t remaining = static_cast<std::int32_t>(queue.size());
      for (const auto i : lp) {
        const auto si = static_cast<std::size_t>(i);
        // drop variable neighbours now covered by the new element; drop absorbed elements; attach the new element
        std::vector<std::int32_t> nv;
        for (const auto v : vadj[si])
          if (alive[static_cast<std::size_t>(v)] && stamp[static_cast<std::size_t>(v)] != token)
            nv.push_back(v);
        vadj[si].swap(nv);
        std::vector<std::int32_t> ne;
        std::int64_t ext = 0;
        for (const auto e : eadj[si])
          if (elem_alive[static_cast<std::size_t>(e)] && e != p) {
            ne.push_back(e);
            ext += static_cast<std::int64_t>(emem[static_cast<std::size_t>(e)].size()) - 1;
          }
        ne.push_back(p);
        eadj[si].swap(ne);
        queue.erase({degree[si], i});
        const std::int64_t d = static_cast<std::int64_t>(vadj[si].size()) + (lsize - 1) + std::max<std::int64_t>(0, ext);
        degree[si] = static_cast<std::int32_t>(std::min<std::int64_t>(d, remaining - 1 > 0 ? remaining - 1 : 0));
        queue.insert({degree[si], i});
      }
    }
    return order;
  }

  bool analysed_{false};
  std::int32_t n_{0};
  std::vector<std::int32_t> order_, inv_, parent_;
  std::vector<std::int64_t> upstart_, lstart_;
  std::vector<std::int32_t> upidx_, lidx_;
  std::vector<double> lval_, diag_;
  Stats stats_{};
};

} // namespace pluss::subsp::math::lp
