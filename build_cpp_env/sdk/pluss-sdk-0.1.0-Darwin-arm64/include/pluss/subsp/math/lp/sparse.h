#pragma once

// Sparse model types for the industry-scale LP / MILP stack (host-side, index-based: sparse pivoting is irregular and does not
// map onto dense Series ops; the dense batched Series solvers in linprog.h remain the right tool for many small related LPs).
//
//   minimise  c^T x + offset   s.t.  row_lo <= A x <= row_hi,   col_lo <= x <= col_hi,   x_j integer for integer[j]
// A is stored column-wise (CSC); infinite bounds are +-infinity. Ranged rows, free rows (-inf, +inf), equalities (lo == hi).

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace pluss::subsp::math::lp {

inline constexpr double kInf = std::numeric_limits<double>::infinity();
// Finite values at or beyond this magnitude (1e30 / 1e300 "big-M infinity" sentinels, as in MPS files and many callers) are treated as infinite,
// the HiGHS convention (kHighsInf = 1e20). Without it such a bound is a genuine huge finite number: it wrecks scaling and can silently change the answer.
inline constexpr double kInfThreshold = 1e20;

struct SparseMatrix final {
  std::int32_t rows{0}, cols{0};
  std::vector<std::int64_t> start{0};   // size cols + 1
  std::vector<std::int32_t> index;      // row index per entry
  std::vector<double> value;

  [[nodiscard]] std::int64_t nnz() const { return static_cast<std::int64_t>(index.size()); }

  // Build from (row, col, value) triplets; duplicates are summed, zeros dropped.
  static SparseMatrix from_triplets(std::int32_t rows, std::int32_t cols, std::vector<std::int32_t> r, std::vector<std::int32_t> c,
                                    std::vector<double> v) {
    SparseMatrix a;
    a.rows = rows;
    a.cols = cols;
    std::vector<std::int64_t> count(static_cast<std::size_t>(cols) + 1, 0);
    for (const auto cc : c)
      ++count[static_cast<std::size_t>(cc) + 1];
    for (std::size_t j = 0; j < static_cast<std::size_t>(cols); ++j)
      count[j + 1] += count[j];
    std::vector<std::int32_t> idx(r.size());
    std::vector<double> val(r.size());
    std::vector<std::int64_t> next(count.begin(), count.end() - 1);
    for (std::size_t k = 0; k < r.size(); ++k) {
      const std::int64_t p = next[static_cast<std::size_t>(c[k])]++;
      idx[static_cast<std::size_t>(p)] = r[k];
      val[static_cast<std::size_t>(p)] = v[k];
    }
    a.start.assign(1, 0);
    std::vector<std::int64_t> pos(static_cast<std::size_t>(rows), -1);
    for (std::int32_t j = 0; j < cols; ++j) {
      const std::int64_t base = static_cast<std::int64_t>(a.index.size());
      for (std::int64_t p = count[static_cast<std::size_t>(j)]; p < count[static_cast<std::size_t>(j) + 1]; ++p) {
        const auto i = idx[static_cast<std::size_t>(p)];
        if (pos[static_cast<std::size_t>(i)] >= base) {
          a.value[static_cast<std::size_t>(pos[static_cast<std::size_t>(i)])] += val[static_cast<std::size_t>(p)];
        } else {
          pos[static_cast<std::size_t>(i)] = static_cast<std::int64_t>(a.index.size());
          a.index.push_back(i);
          a.value.push_back(val[static_cast<std::size_t>(p)]);
        }
      }
      // drop exact zeros created by cancellation
      std::int64_t w = base;
      for (std::int64_t p = base; p < static_cast<std::int64_t>(a.index.size()); ++p)
        if (a.value[static_cast<std::size_t>(p)] != 0.0) {
          a.index[static_cast<std::size_t>(w)] = a.index[static_cast<std::size_t>(p)];
          a.value[static_cast<std::size_t>(w)] = a.value[static_cast<std::size_t>(p)];
          ++w;
        }
      a.index.resize(static_cast<std::size_t>(w));
      a.value.resize(static_cast<std::size_t>(w));
      a.start.push_back(static_cast<std::int64_t>(a.index.size()));
    }
    return a;
  }

  // Row-wise (CSR) copy as another SparseMatrix with rows/cols swapped roles: start has size rows + 1, index holds column ids.
  [[nodiscard]] SparseMatrix transposed() const {
    SparseMatrix t;
    t.rows = cols;
    t.cols = rows;
    std::vector<std::int64_t> count(static_cast<std::size_t>(rows) + 1, 0);
    for (const auto i : index)
      ++count[static_cast<std::size_t>(i) + 1];
    for (std::size_t i = 0; i < static_cast<std::size_t>(rows); ++i)
      count[i + 1] += count[i];
    t.start = count;
    t.index.resize(index.size());
    t.value.resize(value.size());
    std::vector<std::int64_t> next(count.begin(), count.end() - 1);
    for (std::int32_t j = 0; j < cols; ++j)
      for (std::int64_t p = start[static_cast<std::size_t>(j)]; p < start[static_cast<std::size_t>(j) + 1]; ++p) {
        const std::int64_t q = next[static_cast<std::size_t>(index[static_cast<std::size_t>(p)])]++;
        t.index[static_cast<std::size_t>(q)] = j;
        t.value[static_cast<std::size_t>(q)] = value[static_cast<std::size_t>(p)];
      }
    return t;
  }

  // y += alpha * A x   (x of size cols, y of size rows)
  void multiply_add(double alpha, const std::vector<double> &x, std::vector<double> &y) const {
    for (std::int32_t j = 0; j < cols; ++j) {
      const double xj = alpha * x[static_cast<std::size_t>(j)];
      if (xj == 0.0)
        continue;
      for (std::int64_t p = start[static_cast<std::size_t>(j)]; p < start[static_cast<std::size_t>(j) + 1]; ++p)
        y[static_cast<std::size_t>(index[static_cast<std::size_t>(p)])] += xj * value[static_cast<std::size_t>(p)];
    }
  }
  // y += alpha * A^T x
  void multiply_transpose_add(double alpha, const std::vector<double> &x, std::vector<double> &y) const {
    for (std::int32_t j = 0; j < cols; ++j) {
      double s = 0.0;
      for (std::int64_t p = start[static_cast<std::size_t>(j)]; p < start[static_cast<std::size_t>(j) + 1]; ++p)
        s += value[static_cast<std::size_t>(p)] * x[static_cast<std::size_t>(index[static_cast<std::size_t>(p)])];
      y[static_cast<std::size_t>(j)] += alpha * s;
    }
  }
};

struct Model final {
  std::int32_t rows{0}, cols{0};
  std::vector<double> cost;              // size cols
  double offset{0.0};
  SparseMatrix a;                        // rows x cols
  std::vector<double> row_lo, row_hi;    // size rows
  std::vector<double> col_lo, col_hi;    // size cols
  std::vector<char> integer;             // size cols (empty = pure LP)
  std::vector<std::string> row_names, col_names;

  [[nodiscard]] bool has_integers() const {
    for (const char v : integer)
      if (v)
        return true;
    return false;
  }
  [[nodiscard]] bool has_huge_values() const {
    const auto huge = [](double x) { return std::isfinite(x) && std::abs(x) >= kInfThreshold; };
    for (const double x : col_lo) if (huge(x)) return true;
    for (const double x : col_hi) if (huge(x)) return true;
    for (const double x : row_lo) if (huge(x)) return true;
    for (const double x : row_hi) if (huge(x)) return true;
    return false;
  }
  // lower sides <= -1e20 become -inf, upper sides >= 1e20 become +inf (a lower side >= 1e20 or an upper side <= -1e20 is left alone: it is infeasible)
  void canonicalize_infinities() {
    for (double &x : col_lo) if (x <= -kInfThreshold) x = -kInf;
    for (double &x : row_lo) if (x <= -kInfThreshold) x = -kInf;
    for (double &x : col_hi) if (x >= kInfThreshold) x = kInf;
    for (double &x : row_hi) if (x >= kInfThreshold) x = kInf;
  }
  void validate() const {
    if (static_cast<std::int32_t>(cost.size()) != cols || static_cast<std::int32_t>(col_lo.size()) != cols ||
        static_cast<std::int32_t>(col_hi.size()) != cols || static_cast<std::int32_t>(row_lo.size()) != rows ||
        static_cast<std::int32_t>(row_hi.size()) != rows || a.rows != rows || a.cols != cols)
      throw std::invalid_argument("lp::Model: inconsistent dimensions");
  }
};

enum class SolveStatus : int { Optimal = 0, Infeasible = 1, Unbounded = 2, IterationLimit = 3, TimeLimit = 4, InfeasibleOrUnbounded = 5, NodeLimit = 6, ObjectiveLimit = 7 };

} // namespace pluss::subsp::math::lp
