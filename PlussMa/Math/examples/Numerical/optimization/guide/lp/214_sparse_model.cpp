// The sparse solver, for models that are large and mostly zeros. A `lp::Model` is built from triplets (row, column, value), so
// only the non-zeros are stored; rows can be ranged (`row_lo <= row <= row_hi`, with equal ends for an equality), and variables
// have bounds `col_lo` and `col_hi`. Note that this model has no default bounds: every vector must be filled in. The program is a
// transportation problem: three warehouses ship to four shops at a cost per unit, every warehouse ships out exactly its stock and
// every shop receives exactly its demand. It solves the model with `lp::solve`, prints the plan and the prices of the rows, and
// checks the objective against the dense solver on the same data.
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace lp = pluss::subsp::math::lp;
namespace dense = pluss::subsp::math::linprog;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series mat(std::vector<double> v, std::int64_t rows, std::int64_t cols) { return pluss::make_series(v, {rows, cols}).to(f64); }

int main() {
  const int sources = 3, shops = 4;
  const std::vector<double> stock = {30, 25, 45}, demand = {20, 25, 30, 25};
  const std::vector<double> unit_cost = {8, 6, 10, 9,
                                         9, 12, 13, 7,
                                         14, 9, 16, 5};

  // variable x[s * shops + t] = units shipped from warehouse s to shop t; rows 0..2 are the warehouses, rows 3..6 the shops
  lp::Model model;
  model.cols = sources * shops;
  model.rows = sources + shops;
  model.cost = unit_cost;
  std::vector<std::int32_t> row, col;
  std::vector<double> value;
  for (int s = 0; s < sources; ++s)
    for (int t = 0; t < shops; ++t) {
      row.push_back(s);           col.push_back(s * shops + t); value.push_back(1.0);
      row.push_back(sources + t); col.push_back(s * shops + t); value.push_back(1.0);
    }
  model.a = lp::SparseMatrix::from_triplets(model.rows, model.cols, row, col, value);
  model.row_lo = model.row_hi = {};
  for (const double v : stock) { model.row_lo.push_back(v); model.row_hi.push_back(v); }
  for (const double v : demand) { model.row_lo.push_back(v); model.row_hi.push_back(v); }
  model.col_lo.assign(static_cast<std::size_t>(model.cols), 0.0);
  model.col_hi.assign(static_cast<std::size_t>(model.cols), lp::kInf);

  const auto solution = lp::solve(model);
  std::cout << std::fixed << std::setprecision(2) << "status " << (solution.status == lp::SolveStatus::Optimal ? "Optimal" : "other") << ", shipping cost "
            << solution.objective << ", " << model.a.nnz() << " non-zeros\n\nplan (rows are warehouses, columns are shops)\n";
  for (int s = 0; s < sources; ++s) {
    for (int t = 0; t < shops; ++t) std::cout << std::setw(8) << solution.x[static_cast<std::size_t>(s * shops + t)];
    std::cout << "\n";
  }
  std::cout << "\nprices of the rows (warehouses, then shops):";
  for (const double y : solution.row_dual) std::cout << " " << (std::abs(y) < 5e-3 ? 0.0 : y);
  std::cout << "\n";

  // the same program in the dense form
  dense::Problem p;
  p.c = vec(unit_cost);
  std::vector<double> a(static_cast<std::size_t>((sources + shops) * sources * shops), 0.0);
  for (int s = 0; s < sources; ++s)
    for (int t = 0; t < shops; ++t) {
      a[static_cast<std::size_t>(s * (sources * shops) + s * shops + t)] = 1.0;
      a[static_cast<std::size_t>((sources + t) * (sources * shops) + s * shops + t)] = 1.0;
    }
  p.a_eq = mat(a, sources + shops, sources * shops);
  std::vector<double> rhs = stock;
  rhs.insert(rhs.end(), demand.begin(), demand.end());
  p.b_eq = vec(rhs);
  p.lower = vec(std::vector<double>(static_cast<std::size_t>(sources * shops), 0.0));
  std::cout << "dense solver, same data: cost " << dense::solve(p).objective.item<double>() << "\n";
  return 0;
}
