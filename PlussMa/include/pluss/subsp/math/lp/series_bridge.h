#pragma once

// Bridge between the dense Series-based problem descriptions (linprog::Problem / milp::Problem) and the sparse industry-scale stack.
// Use it when a model built from Series is large and sparse: the dense solvers keep every row in memory and factor dense matrices,
// the sparse stack keeps only nonzeros (revised simplex, presolve, interior point, branch-and-cut).
//
//   namespace lp = pluss::subsp::math::lp;
//   auto sol = lp::solve_sparse(problem);                // linprog::Problem -> LpSolution (single objective lane)
//   auto res = lp::solve_sparse(milp_problem, options);  // milp::Problem   -> MipResult

#include "pluss/subsp/math/lp/mip.h"
#include "pluss/subsp/math/lp/solve.h"
#include "pluss/subsp/math/milp.h"

namespace pluss::subsp::math::lp {

// Dense [rows, n] Series -> sparse triplets appended to the model (rows are numbered from `row0`).
inline void append_dense_rows_(const Series &a, std::int32_t n, std::int32_t row0, std::vector<std::int32_t> &r, std::vector<std::int32_t> &c,
                               std::vector<double> &v) {
  const auto data = a.to(pluss::ScalarType::Float64).contiguous().to_vec<double>();
  const auto rows = static_cast<std::int32_t>(data.size() / static_cast<std::size_t>(n));
  for (std::int32_t i = 0; i < rows; ++i)
    for (std::int32_t j = 0; j < n; ++j) {
      const double x = data[static_cast<std::size_t>(i) * static_cast<std::size_t>(n) + static_cast<std::size_t>(j)];
      if (x != 0.0) {
        r.push_back(row0 + i);
        c.push_back(j);
        v.push_back(x);
      }
    }
}

[[nodiscard]] inline std::vector<double> dense_vec_(const Series &s) { return s.to(pluss::ScalarType::Float64).contiguous().to_vec<double>(); }

// Single-lane conversion: c must be [n]; rows are a_ub x <= b_ub then a_eq x == b_eq. Missing bounds default to [0, +inf).
[[nodiscard]] inline Model to_sparse(const linprog::Problem &p, const Series &integer = pluss::series::undefined()) {
  Model M;
  M.cost = dense_vec_(p.c);
  const auto n = static_cast<std::int32_t>(M.cost.size());
  M.cols = n;
  std::vector<std::int32_t> r, c;
  std::vector<double> v;
  std::int32_t rows = 0;
  if (p.a_ub) {
    append_dense_rows_(*p.a_ub, n, rows, r, c, v);
    const auto b = dense_vec_(*p.b_ub);
    for (const double bi : b) {
      M.row_lo.push_back(-kInf);
      M.row_hi.push_back(bi);
    }
    rows += static_cast<std::int32_t>(b.size());
  }
  if (p.a_eq) {
    append_dense_rows_(*p.a_eq, n, rows, r, c, v);
    const auto b = dense_vec_(*p.b_eq);
    for (const double bi : b) {
      M.row_lo.push_back(bi);
      M.row_hi.push_back(bi);
    }
    rows += static_cast<std::int32_t>(b.size());
  }
  M.rows = rows;
  M.a = SparseMatrix::from_triplets(rows, n, r, c, v);
  M.col_lo = p.lower ? dense_vec_(*p.lower) : std::vector<double>(static_cast<std::size_t>(n), 0.0);
  M.col_hi = p.upper ? dense_vec_(*p.upper) : std::vector<double>(static_cast<std::size_t>(n), kInf);
  if (integer.defined()) {
    const auto flags = dense_vec_(integer);
    M.integer.assign(static_cast<std::size_t>(n), 0);
    for (std::size_t j = 0; j < flags.size(); ++j)
      M.integer[j] = flags[j] != 0.0;
  }
  M.validate();
  return M;
}

[[nodiscard]] inline Model to_sparse(const milp::Problem &p) { return to_sparse(p.lp, p.integer); }

// Presolve + sparse simplex on a linprog::Problem (one objective lane).
[[nodiscard]] inline LpSolution solve_sparse(const linprog::Problem &p, const LpOptions &opt = {}) { return solve(to_sparse(p), opt); }

// Presolve + branch-and-cut on a milp::Problem.
[[nodiscard]] inline MipResult solve_sparse(const milp::Problem &p, const MipOptions &opt = {}) { return solve_mip(to_sparse(p), opt); }

} // namespace pluss::subsp::math::lp
