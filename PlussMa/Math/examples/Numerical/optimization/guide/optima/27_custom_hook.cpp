// A progress hook is any function from a snapshot to a bool. It sees the stage (start, each
// iteration, done), the current point, the evaluation there, and the work counters. Returning true
// asks the solver to stop at the next opportunity. This hook records the objective history and stops
// the run as soon as the point is feasible and the objective is below 10.55.
#include <pluss/ma.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace optim = pluss::subsp::math::optim;
  using Pack = optim::ParameterPack<pluss::Series>;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  auto objective = [](const pluss::Series& x) { return (x - 3.0).square().sum(); };
  const auto disc = optim::constraints().nl_ineq(
      [](const Pack& p) { return p.at("x").square().sum() - 1.0; });

  std::vector<double> history;
  optim::OptimaOptions options;
  options.progress_hook = [&](const optim::OptimaProgressSnapshot& s) {
    const double value = s.evaluation.value.item<double>();
    std::cout << std::string(optim::optima_progress_stage_name(s.stage)) << "  iteration " << s.iteration
              << "  objective " << value << "  violation " << s.evaluation.max_constraint_violation << "\n";
    if (s.stage == optim::OptimaProgressStage::Iteration) history.push_back(value);
    return s.stage == optim::OptimaProgressStage::Iteration && s.evaluation.max_constraint_violation <= 1e-6 && value < 10.55;
  };

  const auto r = optim::optima(objective, vec({0.0, 0.0}), options, disc);
  std::cout << "recorded " << history.size() << " iteration values\n";
  std::cout << "exit flag " << static_cast<int>(r.exit_flag) << ", stopped at iteration " << r.iteration << "\n";
  const auto x = r.params.at("x").host<double>();
  std::cout << "returned point [" << x[0] << ", " << x[1] << "]\n";
  return 0;
}
