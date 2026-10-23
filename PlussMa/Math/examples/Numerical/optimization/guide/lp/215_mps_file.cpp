// Models often arrive as MPS files, the long-standing exchange format for linear and integer programs. `read_mps` reads fixed or
// free format: ROWS with their types (N objective, L <=, G >=, E =), COLUMNS, RHS, RANGES, BOUNDS, and INTORG/INTEND markers for
// integer columns. Here a small model is held in a string, read through a stream, solved, and reported with the names from the
// file. A model that maximizes is returned as the negated minimization with `negated_objective` set, so the reported value is
// flipped back. (A file is read the same way from disk with `read_mps_file(path)`.)
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
