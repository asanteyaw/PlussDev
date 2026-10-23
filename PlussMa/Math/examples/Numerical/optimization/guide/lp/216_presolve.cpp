// Presolve simplifies a model before the solver sees it, and the answer is unchanged. It removes rows and columns that cannot
// matter: a row with a single variable becomes a bound, a variable with equal bounds is fixed and substituted out, an empty row is
// dropped, and a row that the bounds already guarantee is redundant. The program builds a model with each of these hidden in it,
// runs `lp::presolve` and reports how many rows and columns it removed, and solves the original with presolve on and off to show that
// the optimal value is the same. The solution that comes back is always for the original model, with its own numbering.
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
