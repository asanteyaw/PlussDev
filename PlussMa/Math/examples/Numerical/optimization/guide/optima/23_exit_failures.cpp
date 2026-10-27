// The flags that mean the solver could not deliver a solution. Each comes with the program
// state that triggers it, and the result still carries the last point reached.
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

#include <string>
#include <utility>
#include <vector>

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;
using namespace pluss::tils;

int main() {
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  std::vector<std::pair<std::string, optim::OptimaState>> runs;
  auto objective = [](const pluss::Series& x) { return (x - 3.0).square().sum(); };
  // No point satisfies x0^2 + x1^2 + 1 <= 0, and none satisfies x0^2 + x1^2 + 1 = 0.
  const auto impossible_inequality = optim::constraints().nl_ineq(
      [](const Pack& p) { return p.at("x").square().sum() + 1.0; });
  const auto impossible_equality = optim::constraints().nl_eq(
      [](const Pack& p) { return p.at("x").square().sum() + 1.0; });

  // NoFeasiblePoint: the interior method keeps bounds and linear rows strictly satisfied, so it first looks for
  // a point inside them. Here x0 + x1 <= -1 contradicts x >= 0, and it finds none.
  optim::OptimaOptions interior;
  interior.algorithm = optim::OptimaAlgorithm::InternalProtocol;
  const auto impossible_linear = optim::constraints()
                                     .lbs(vec({0.0, 0.0}))
                                     .ln_ineq(pluss::make_series(std::vector<double>{1.0, 1.0}, {1, 2}).to(f64), vec({-1.0}));
  runs.emplace_back("interior method, impossible linear rows", optim::optima(objective, vec({1.0, 1.0}), interior, impossible_linear));

  // ConstraintStationary also covers a nonlinear inequality that no point satisfies.
  runs.emplace_back("interior method, impossible inequality", optim::optima(objective, vec({0.0, 0.0}), interior, impossible_inequality));

  // NoProgress: the recovery step could not reduce the violation, so the run gave up.
  runs.emplace_back("sequential QP, impossible inequality", optim::optima(objective, vec({0.0, 0.0}), {}, impossible_inequality));

  // ConstraintStationary: the violation stopped decreasing at a point that is still infeasible.
  runs.emplace_back("interior method, impossible equality", optim::optima(objective, vec({1.0, 1.0}), interior, impossible_equality));

  // NonFiniteEvaluation: the objective is not a number at the start (log of a negative value).
  runs.emplace_back("log of a negative start",
         optim::optima([](const pluss::Series& x) { return x.log().sum(); }, vec({-1.0, 1.0})));

  // InvalidProblem: the objective returned two values instead of one.
  runs.emplace_back("objective returns a vector", optim::optima([](const pluss::Series& x) { return x.square(); }, vec({1.0, 1.0})));

  OutputEngine out;
  for (const auto& [label, r] : runs) out.writeln(label + ":", optim::optima_exit_flag_name(r.exit_flag), "code", static_cast<int>(r.exit_flag), "max violation", r.evaluation.max_constraint_violation);
  return 0;
}
