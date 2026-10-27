// The exit flag says why the solver stopped. This program triggers the four flags that mean the run
// was healthy: it finished (Converged), it ran out of iterations (MaxIterations), the caller's hook
// asked it to stop (Stopped), or it reached an objective value the caller said was good enough
// (ObjectiveTargetReached).
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

#include <string>
#include <utility>
#include <vector>

namespace optim = pluss::subsp::math::optim;
using namespace pluss::tils;

int main() {
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  auto objective = [](const pluss::Series& x) { return (x - 3.0).square().sum(); };
  using Pack = optim::ParameterPack<pluss::Series>;
  const auto disc = optim::constraints().nl_ineq(
      [](const Pack& p) { return p.at("x").square().sum() - 1.0; });

  std::vector<std::pair<std::string, optim::OptimaState>> runs;

  // Converged: the usual outcome.
  runs.emplace_back("finished normally", optim::optima(objective, vec({0.0, 0.0})));

  // MaxIterations: the iteration budget is spent before the tolerances are met.
  optim::OptimaOptions few;
  few.sequential_qp.max_iterations = 1;
  runs.emplace_back("one iteration allowed", optim::optima(objective, vec({0.0, 0.0}), few, disc));

  // Stopped: the progress hook returned true, asking for an orderly stop.
  optim::OptimaOptions stop_early;
  int calls = 0;
  stop_early.progress_hook = [&](const optim::OptimaProgressSnapshot&) { return ++calls >= 2; };
  runs.emplace_back("hook asked to stop", optim::optima(objective, vec({0.0, 0.0}), stop_early));

  // ObjectiveTargetReached: a feasible point at or below objective_limit ends the run at once.
  optim::OptimaOptions good_enough;
  good_enough.sequential_qp.objective_limit = 100.0;
  runs.emplace_back("objective limit 100", optim::optima(objective, vec({0.0, 0.0}), good_enough));

  OutputEngine out;
  for (const auto& [label, r] : runs)
    out.writeln(label + ":", optim::optima_exit_flag_name(r.exit_flag), "code", static_cast<int>(r.exit_flag), "converged", r.converged ? "true" : "false", "iterations", r.iteration);
  return 0;
}
