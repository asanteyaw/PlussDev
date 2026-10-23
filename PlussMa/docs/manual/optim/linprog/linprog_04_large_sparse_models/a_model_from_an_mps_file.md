<!-- generated from a_model_from_an_mps_file.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# A model from an MPS file

[Back to Large Sparse Models](../linprog_04_large_sparse_models.md)


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
