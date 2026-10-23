<!-- generated from a_transportation_model.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# A transportation model

[Back to Large Sparse Models](../linprog_04_large_sparse_models.md)


Three warehouses ship to four shops. Every warehouse ships out exactly its stock, and every shop receives exactly its demand, so the model has 12 variables, 7 equality rows and 24 non-zeros: each shipment appears in one warehouse row and one shop row.

```cpp
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
```

Output

```text
status Optimal, shipping cost 830.00, 24 non-zeros

plan (rows are warehouses, columns are shops)
    0.00    5.00   25.00    0.00
   20.00    0.00    5.00    0.00
    0.00   20.00    0.00   25.00

prices of the rows (warehouses, then shops): 6.00 9.00 9.00 0.00 0.00 4.00 -4.00
dense solver, same data: cost 830.00
```
The cheapest plan costs 830. It ships 5 from warehouse 0 to shop 1 and 25 to shop 2, 20 from warehouse 1 to shop 0 and 5 to shop 2, and 20 and 25 from warehouse 2 to shops 1 and 3. Every shipment is a whole number, which is a property of transportation models: the constraint matrix is such that the optimal corners are integral, so there is no need for an integer program. The dense solver given the same data finds the same cost, 830.

The prices of the rows are `6, 9, 9` for the warehouses and `0, 0, 4, -4` for the shops. Read them with one caution: because the total stock equals the total demand, one row is redundant, and the prices are determined only up to adding a constant to every warehouse price and subtracting it from every shop price. The *differences* are the meaningful part. Shops 2 and 3 (numbering from 0) are priced 4 and -4; raising the demand of shop 2 by one unit while lowering the demand of shop 3 by one raises the cost by 4 - (-4) = 8, which re-solving with the demands shifted confirms.
