// The search step. Before polling, pattern search may try extra points chosen to jump ahead: ExpandAlongSuccess
// extends the last successful step by `search_expansion`, RandomDirections tries `search_trials` random points around
// the best, ExpandAndRandomDirections does both. A successful search skips the poll for that iteration. The diagnostics
// count the trials and how many succeeded.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series rosenbrock(const pluss::Series& x) {
  const auto a = x.select(0, 0), b = x.select(0, 1);
  return (b - a * a).square() * 100.0 + (a * -1.0 + 1.0).square();
}

int main() {
  struct S { const char* name; dfo::PSSearchStrategy s; };
  const S rows[] = {{"Off", dfo::PSSearchStrategy::Off},
                    {"ExpandAlongSuccess (default)", dfo::PSSearchStrategy::ExpandAlongSuccess},
                    {"RandomDirections", dfo::PSSearchStrategy::RandomDirections},
                    {"ExpandAndRandomDirections", dfo::PSSearchStrategy::ExpandAndRandomDirections}};
  std::cout << std::left << std::setw(32) << "search strategy" << std::setw(26) << "status" << std::setw(11) << "minimum" << std::setw(8) << "evals"
            << "trials / successes\n";
  for (const auto& r : rows) {
    dfo::PSConfig c;
    c.search_strategy = r.s;
    const auto o = dfo::pattern_search_minimizer(rosenbrock, vec({-1.2, 1.0}), c);
    std::cout << std::left << std::setw(32) << r.name << std::setw(26) << dfo::to_string(o.status) << std::scientific << std::setprecision(2)
              << std::setw(11) << o.minimum << std::defaultfloat << std::setw(8) << o.diagnostics.evaluations << o.diagnostics.search_trials
              << " / " << o.diagnostics.search_successes << "\n";
  }
}
