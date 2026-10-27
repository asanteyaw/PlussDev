// A problem whose variables live on very different scales is the classic stress test for a solver that takes
// one step length for every variable. The objective here is 1e8 times steeper in `a` than in `b`, the two
// variables differ by a factor of four thousand in size, and the equality constraint is written in units of
// 1e-6. This program solves it with every algorithm and every `problem_scaling` mode and prints the
// iteration counts. Read the table for what scaling buys: all of them succeed, and the mode matters little.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::subsp::math::optim;
  using Pack = optim::ParameterPack<pluss::Series>;
  const auto f64 = pluss::ScalarType::Float64;
  auto scalar = [&](double v) { return pluss::make_series(v).to(f64); };
  auto two = [&](double a, double b) { return optim::pack_scalars({{"a", scalar(a)}, {"b", scalar(b)}}); };

  // minimize 1e8 (a - 0.3)^2 + 1e-4 (b - 1250)^2  subject to  1e-6 (a + b - 1250.3) = 0  and  a, b >= 0
  auto objective = [](const Pack& p) {
    return 1.0e8 * (p.at("a") - 0.3).square() + 1.0e-4 * (p.at("b") - 1250.0).square();
  };
  const auto constraints =
      optim::constraints()
          .nl_eq([](const Pack& p) { return 1.0e-6 * (p.at("a") + p.at("b") - 1250.3); })
          .lbs(two(0.0, 0.0));

  struct Mode { const char* label; optim::OptimaProblemScaling scaling; };
  const std::vector<Mode> modes = {{"None", optim::OptimaProblemScaling::None},
                                   {"InitialPoint", optim::OptimaProblemScaling::InitialPoint},
                                   {"ObjectiveAndConstraints", optim::OptimaProblemScaling::ObjectiveAndConstraints}};
  struct Row { const char* label; optim::OptimaAlgorithm algorithm; };
  const std::vector<Row> rows = {{"MeritMethod", optim::OptimaAlgorithm::MeritMethod},
                                 {"SequentialQP", optim::OptimaAlgorithm::SequentialQP},
                                 {"WorkingSet", optim::OptimaAlgorithm::WorkingSet},
                                 {"InternalProtocol", optim::OptimaAlgorithm::InternalProtocol}};

  std::cout << std::left << std::setw(18) << "iterations (flag)";
  for (const auto& m : modes) std::cout << std::setw(26) << m.label;
  std::cout << "\n";
  for (const auto& row : rows) {
    std::cout << std::left << std::setw(18) << row.label;
    for (const auto& mode : modes) {
      optim::OptimaOptions options;
      options.algorithm = row.algorithm;
      options.sequential_qp.problem_scaling = mode.scaling;
      options.working_set.problem_scaling = mode.scaling;
      options.internal_protocol.problem_scaling = mode.scaling;
      const auto r = optim::optima(objective, two(1.0, 1000.0), options, constraints);
      std::cout << std::setw(26) << (std::to_string(r.iteration) + " (" + std::to_string(static_cast<int>(r.exit_flag)) + ")");
    }
    std::cout << "\n";
  }
  return 0;
}
