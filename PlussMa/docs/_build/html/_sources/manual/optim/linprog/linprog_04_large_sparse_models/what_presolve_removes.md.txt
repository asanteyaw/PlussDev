<!-- generated from what_presolve_removes.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# What presolve removes

[Back to Large Sparse Models](../linprog_04_large_sparse_models.md)


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
