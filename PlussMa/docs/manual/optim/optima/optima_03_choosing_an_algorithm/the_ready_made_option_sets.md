<!-- generated from the_ready_made_option_sets.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The ready-made option sets

[Back to Choosing and Tuning an Algorithm](../optima_03_choosing_an_algorithm.md)


Three presets return a complete `OptimaOptions` with the algorithm chosen and the tolerances set for a typical constrained fit. The program prints what they change from the defaults and runs each on the problem from Program 1.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace optim = pluss::optim;

const char* algorithm_name(optim::OptimaAlgorithm a) {
  switch (a) {
    case optim::OptimaAlgorithm::MeritMethod: return "MeritMethod";
    case optim::OptimaAlgorithm::SequentialQP: return "SequentialQP";
    case optim::OptimaAlgorithm::WorkingSet: return "WorkingSet";
    case optim::OptimaAlgorithm::InternalProtocol: return "InternalProtocol";
  }
  return "?";
}

const char* scaling_name(optim::OptimaProblemScaling s) {
  switch (s) {
    case optim::OptimaProblemScaling::None: return "None";
    case optim::OptimaProblemScaling::InitialPoint: return "InitialPoint";
    case optim::OptimaProblemScaling::ObjectiveAndConstraints: return "ObjectiveAndConstraints";
  }
  return "?";
}

int main() {
  using Pack = optim::ParameterPack<optim::Trace>;
  const auto f64 = pluss::ScalarType::Float64;
  auto scalar = [&](double v) { return pluss::make_series(v).to(f64); };
  auto four = [&](double a, double b, double c, double d) {
    return optim::pack_scalars({{"a", scalar(a)}, {"b", scalar(b)}, {"c", scalar(c)}, {"d", scalar(d)}});
  };
  auto objective = [](const Pack& p) {
    return p.at("a") * p.at("d") * (p.at("a") + p.at("b") + p.at("c")) + p.at("c");
  };
  const auto constraints =
      optim::constraints()
          .nl_ineq([](const Pack& p) { return 25.0 - p.at("a") * p.at("b") * p.at("c") * p.at("d"); })
          .nl_eq([](const Pack& p) {
            return optim::square(p.at("a")) + optim::square(p.at("b")) + optim::square(p.at("c")) +
                   optim::square(p.at("d")) - 40.0;
          })
          .lbs(four(1, 1, 1, 1))
          .ubs(four(5, 5, 5, 5));

  const optim::OptimaOptions defaults;
  struct Preset { const char* label; optim::OptimaOptions options; };
  const std::vector<Preset> presets = {{"defaults", defaults},
                                       {"sequential_qp()", optim::presets::sequential_qp()},
                                       {"internal_protocol()", optim::presets::internal_protocol()},
                                       {"robust_internal_protocol()", optim::presets::robust_internal_protocol()}};

  std::cout << std::left << std::setw(28) << "options" << std::setw(18) << "algorithm" << std::setw(12)
            << "max iters" << std::setw(16) << "constraint tol" << std::setw(13) << "optimal tol" << "scaling\n";
  for (const auto& p : presets) {
    const auto& o = p.options;
    const bool interior = o.algorithm == optim::OptimaAlgorithm::InternalProtocol;
    const auto iters = interior ? o.internal_protocol.max_iterations : o.sequential_qp.max_iterations;
    const auto ctol = interior ? o.internal_protocol.constraint_tolerance : o.sequential_qp.constraint_tolerance;
    const auto otol = interior ? o.internal_protocol.optimality_tolerance : o.sequential_qp.optimality_tolerance;
    const auto scaling = interior ? o.internal_protocol.problem_scaling : o.sequential_qp.problem_scaling;
    std::cout << std::left << std::setw(28) << p.label << std::setw(18) << algorithm_name(o.algorithm)
              << std::setw(12) << iters << std::setw(16) << ctol << std::setw(13) << otol
              << scaling_name(scaling) << "\n";
  }

  std::cout << "\nrunning each on the same problem\n";
  for (const auto& p : presets) {
    const auto r = optim::optima(objective, four(1, 5, 5, 1), p.options, constraints);
    std::cout << "  " << std::left << std::setw(28) << p.label << "flag " << std::setw(3)
              << static_cast<int>(r.exit_flag) << "iterations " << std::setw(4) << r.iteration << "violation "
              << std::scientific << std::setprecision(1) << r.evaluation.max_constraint_violation
              << std::defaultfloat << "\n";
  }
  return 0;
}
```

Output

```text
options                     algorithm         max iters   constraint tol  optimal tol  scaling
defaults                    SequentialQP      100         1e-06           1e-06        ObjectiveAndConstraints
sequential_qp()             SequentialQP      100         1e-08           1e-06        InitialPoint
internal_protocol()         InternalProtocol  100         1e-08           1e-06        InitialPoint
robust_internal_protocol()  InternalProtocol  160         1e-08           1e-06        InitialPoint

running each on the same problem
  defaults                    flag 1  iterations 4   violation 8.2e-08
  sequential_qp()             flag 1  iterations 5   violation 3.6e-11
  internal_protocol()         flag 1  iterations 9   violation 5.3e-12
  robust_internal_protocol()  flag 1  iterations 9   violation 5.3e-12
```
The presets differ from the defaults in tighter constraint tolerance (`1e-8` against `1e-6`) and in turning on scaling from the starting point. `robust_internal_protocol()` raises the iteration limit to 160 and widens the safeguards, so it matters on problems that need more room, not on this one. If you start from a preset and then change one setting, you keep the rest of its choices.
