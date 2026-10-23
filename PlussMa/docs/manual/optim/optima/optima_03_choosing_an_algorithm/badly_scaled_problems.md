<!-- generated from badly_scaled_problems.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Badly scaled problems

[Back to Choosing and Tuning an Algorithm](../optima_03_choosing_an_algorithm.md)


A problem whose variables or constraints are written in very different units is the classic stress test for step-based methods. This one is 1e8 times steeper in one variable than in the other, has variables four thousand times apart in size, and states its equality in units of 1e-6. The table solves it with every algorithm under each `problem_scaling` mode.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::optim;
  using Pack = optim::ParameterPack<optim::Trace>;
  const auto f64 = pluss::ScalarType::Float64;
  auto scalar = [&](double v) { return pluss::make_series(v).to(f64); };
  auto two = [&](double a, double b) { return optim::pack_scalars({{"a", scalar(a)}, {"b", scalar(b)}}); };

  // minimize 1e8 (a - 0.3)^2 + 1e-4 (b - 1250)^2  subject to  1e-6 (a + b - 1250.3) = 0  and  a, b >= 0
  auto objective = [](const Pack& p) {
    return 1.0e8 * optim::square(p.at("a") - 0.3) + 1.0e-4 * optim::square(p.at("b") - 1250.0);
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
```

Output

```text
iterations (flag) None                      InitialPoint              ObjectiveAndConstraints   
MeritMethod       9 (1)                     9 (1)                     9 (1)                     
SequentialQP      2 (1)                     2 (1)                     3 (1)                     
WorkingSet        2 (1)                     2 (1)                     3 (1)                     
InternalProtocol  5 (1)                     5 (1)                     6 (1)                     
```
Every one of the twelve runs converges, in at most nine iterations. The scaling mode changes the iteration count by at most one, so on this problem it is not a decision you need to make. (The merit method has no scaling option, so its row is the same in every column.) The default mode, `ObjectiveAndConstraints`, solves a rescaled copy of your problem, with the objective and each constraint multiplied by a constant that brings its gradient at the start to a reasonable size, and reports the answer in your own units; that, together with judging optimality relative to the size of the starting gradient, is why the units of your constraints do not decide whether a run succeeds. Chapter 6 measures it on problems written in far more extreme units than this one.
