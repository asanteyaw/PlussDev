// The same problem through the three entry points: `solve_dfo` (objective plus start), `DFO::solve` (a `DfoProblem`
// and `DfoOptions` you fill in), and `DfoBuilder` (chained setters). They run the same method with the same seed, so
// the answers and the evaluation counts are identical.
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <utility>
#include <vector>

namespace dfo = pluss::subsp::math::optimization;
using namespace pluss::tils;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series objective(const pluss::Series& x) { return (x - vec({1.0, -2.0})).square().sum() + 3.0; }

int main() {
  const auto start = vec({4.0, 4.0});

  dfo::DfoProblem problem;
  problem.initial = start;
  problem.objective = objective;

  std::vector<std::pair<const char*, dfo::DfoResult>> runs;
  runs.emplace_back("solve_dfo", dfo::solve_dfo(objective, start));
  runs.emplace_back("DFO::solve", dfo::DFO::solve(problem, dfo::DfoOptions{}));
  runs.emplace_back("DfoBuilder", dfo::DFO::build(start).with_objective(objective).solve());

  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  for (const auto& [label, r] : runs) out.writeln(label, dfo::to_string(r.status), dfo::to_string(r.selected_solver), "minimum", r.minimum, "evals", r.diagnostics.evaluations);
}
