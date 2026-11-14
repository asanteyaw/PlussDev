<!-- generated from large_sparse_models.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Large sparse models

[Back to Linear Programming 4: Large Sparse Models](../linprog_04_large_sparse_models.md)

The dense solver of the earlier chapters keeps every entry of the constraint matrix, which is the right design for many small related programs. A large real model is different: a transportation network, a production plan across plants and weeks, a supply chain with thousands of products. Most of its coefficients are zero, because each constraint mentions only a few variables. The sparse solver stores and factorises only the non-zeros.

The page shows the three things you do with such a model. You **build** one (a transportation problem with 12 variables and 24 non-zeros), you **read** one (an MPS file, the standard exchange format, with its names, senses and bounds), and you **let presolve simplify it** before the solver sees it: a row with one variable becomes a bound, a variable with equal bounds is substituted out, an empty row is dropped, a row the bounds already guarantee is dropped. Presolve returns the same solution, faster.

## Program 1: A transportation model

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

## Program 2: A model from an MPS file

Models usually arrive as MPS files. `read_mps` reads fixed or free format, with objective senses, ranges, bounds, and integer markers.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <sstream>

namespace lp = pluss::subsp::math::lp;

int main() {
  const std::string text =
      "NAME          BLEND\n"
      "OBJSENSE\n"
      "    MAX\n"
      "ROWS\n"
      " N  PROFIT\n"
      " L  MACHINE\n"
      " L  LABOUR\n"
      " G  MINIMUM\n"
      "COLUMNS\n"
      "    CHAIRS    PROFIT         20.0   MACHINE         3.0\n"
      "    CHAIRS    LABOUR          2.0   MINIMUM         1.0\n"
      "    TABLES    PROFIT         30.0   MACHINE         4.0\n"
      "    TABLES    LABOUR          5.0   MINIMUM         1.0\n"
      "RHS\n"
      "    RHS       MACHINE       120.0   LABOUR         100.0\n"
      "    RHS       MINIMUM         5.0\n"
      "BOUNDS\n"
      " UP BND       CHAIRS         25.0\n"
      "ENDATA\n";
  std::istringstream in(text);
  const auto mps = lp::read_mps(in);
  const auto& model = mps.model;
  const auto solution = lp::solve(model);

  std::cout << std::fixed << std::setprecision(3);
  std::cout << "model " << mps.name << ": " << model.rows << " rows, " << model.cols << " columns, " << model.a.nnz() << " non-zeros\n";
  std::cout << "status " << (solution.status == lp::SolveStatus::Optimal ? "Optimal" : "other") << ", profit "
            << (mps.negated_objective ? -solution.objective : solution.objective) << "\n";
  for (int j = 0; j < model.cols; ++j) std::cout << "  " << model.col_names[static_cast<std::size_t>(j)] << " = " << solution.x[static_cast<std::size_t>(j)] << "\n";
  return 0;
}
```

Output

```text
model BLEND: 3 rows, 2 columns, 6 non-zeros
status Optimal, profit 800.000
  CHAIRS = 25.000
  TABLES = 10.000
```
The file is held in a string here and read through a stream; `read_mps_file(path)` reads from disk. The model has 3 rows, 2 columns and 6 non-zeros, and the names from the file come back with it. The file asks to *maximize*, and the reader returns the negated minimization with `negated_objective` set, so the program flips the sign back: the profit is 800, at 25 chairs (at their upper bound) and 10 tables. You can check it by hand: the 25 chairs use 75 of the 120 machine hours and 50 of the 100 labour hours, and the labour limit then allows 10 tables.

## Program 3: What presolve removes

Presolve simplifies the model before the solver sees it. It turns a row with one variable into a bound, substitutes out a variable whose bounds are equal, drops an empty row, and drops a row that the bounds already guarantee. The program builds a model with each of these hidden in it.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace lp = pluss::subsp::math::lp;

int main() {
  // minimize  x0 + 2 x1 + 3 x2 + 4 x3 + 5 x4
  //   row 0:  x0 + x1 + x2 + x3 + x4 >= 6        (the real constraint)
  //   row 1:  x2 <= 4                              (a singleton row: really a bound)
  //   row 2:  (empty)  0 <= 5                      (no variables at all)
  //   row 3:  x0 + x1 <= 100                       (redundant: x0, x1 <= 10)
  // x3 is fixed at 1 (equal bounds); all bounds 0 <= x <= 10 otherwise
  lp::Model model;
  model.cols = 5;
  model.rows = 4;
  model.cost = {1, 2, 3, 4, 5};
  std::vector<std::int32_t> r, c;
  std::vector<double> v;
  for (int j = 0; j < 5; ++j) { r.push_back(0); c.push_back(j); v.push_back(1.0); }
  r.push_back(1); c.push_back(2); v.push_back(1.0);
  r.push_back(3); c.push_back(0); v.push_back(1.0);
  r.push_back(3); c.push_back(1); v.push_back(1.0);
  model.a = lp::SparseMatrix::from_triplets(4, 5, r, c, v);
  model.row_lo = {6.0, -lp::kInf, -lp::kInf, -lp::kInf};
  model.row_hi = {lp::kInf, 4.0, 5.0, 100.0};
  model.col_lo = {0, 0, 0, 1, 0};
  model.col_hi = {10, 10, 10, 1, 10};

  const auto reduction = lp::presolve(model);
  std::cout << "presolve removed " << reduction.removed_rows << " of " << model.rows << " rows and " << reduction.removed_cols << " of " << model.cols
            << " columns, in " << reduction.passes << " passes\n\n";

  std::cout << std::fixed << std::setprecision(4);
  for (const bool use_presolve : {true, false}) {
    lp::LpOptions options;
    options.presolve.enabled = use_presolve;
    const auto s = lp::solve(model, options);
    std::cout << "presolve " << std::left << std::setw(4) << (use_presolve ? "on" : "off") << " objective " << s.objective << "   x =";
    for (const double x : s.x) std::cout << " " << x;
    std::cout << "\n";
  }
  return 0;
}
```

Output

```text
presolve removed 3 of 4 rows and 1 of 5 columns, in 2 passes

presolve on   objective 9.0000   x = 5.0000 0.0000 0.0000 1.0000 0.0000
presolve off  objective 9.0000   x = 5.0000 0.0000 0.0000 1.0000 0.0000
```
`lp::presolve` removes 3 of the 4 rows and 1 of the 5 columns, in 2 passes: the singleton row `x2 <= 4`, the empty row, the redundant row `x0 + x1 <= 100`, and the fixed column `x3 = 1`. The solution with presolve on and off is the same, objective 9 at `(5, 0, 0, 1, 0)`, and it is reported in the numbering of the original model, with all five columns, even though the solver worked on a model with one row. Leave presolve on. Turn it off only to diagnose a surprising result: if the answer changes, report it.

## Decisions and pitfalls

- **Transportation models have whole-number solutions.** The constraint matrix has a property that makes the optimum integral without asking.
- **Prices are determined up to a constant on a redundant model.** When total stock equals total demand, one row is redundant, so the dual values are unique only up to adding a constant to the warehouse prices and subtracting it from the shop prices.
- **An MPS file that asks to maximise comes back negated.** The reader returns the minimisation with a flag; read the sense before interpreting the objective.
- **Sparse variables are non-negative by default.** The dense solver's are free; check the bounds when moving a model between them.
- **Leave presolve on.** It changes the size of the problem and not the answer; turn it off only to debug.

Where next: [Integer programming](../../milp.md) adds whole-number decisions.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
