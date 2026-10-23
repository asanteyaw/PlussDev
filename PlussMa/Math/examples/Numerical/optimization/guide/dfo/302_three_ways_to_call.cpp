// The same problem through the three entry points: `solve_dfo` (objective plus start), `DFO::solve` (a `DfoProblem`
// and `DfoOptions` you fill in), and `DfoBuilder` (chained setters). They run the same method with the same seed, so
// the answers and the evaluation counts are identical.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series objective(const pluss::Series& x) { return (x - vec({1.0, -2.0})).square().sum() + 3.0; }

void report(const char* label, const dfo::DfoResult& r) {
  std::cout << std::left << std::setw(14) << label << std::setw(26) << dfo::to_string(r.status) << std::setw(12)
            << dfo::to_string(r.selected_solver) << std::fixed << std::setprecision(6) << std::setw(12) << r.minimum
            << "evals " << r.diagnostics.evaluations << "\n";
}

int main() {
  const auto start = vec({4.0, 4.0});

  report("solve_dfo", dfo::solve_dfo(objective, start));

  dfo::DfoProblem problem;
  problem.initial = start;
  problem.objective = objective;
  report("DFO::solve", dfo::DFO::solve(problem, dfo::DfoOptions{}));

  report("DfoBuilder", dfo::DFO::build(start).with_objective(objective).solve());
}
