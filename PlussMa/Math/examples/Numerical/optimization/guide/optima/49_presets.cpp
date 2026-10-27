// Ready-made option sets. `presets::sequential_qp()`, `presets::internal_protocol()` and
// `presets::robust_internal_protocol()` return a complete `OptimaOptions` with the algorithm already chosen
// and the tolerances and safeguards set for a typical constrained fit. This program prints the fields each
// preset sets that differ from a default `OptimaOptions`, then runs all three on one problem.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace optim = pluss::subsp::math::optim;

int main() {
  using Pack = optim::ParameterPack<pluss::Series>;
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
            return p.at("a").square() + p.at("b").square() + p.at("c").square() +
                   p.at("d").square() - 40.0;
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
    std::cout << std::left << std::setw(28) << p.label << std::setw(18) << optim::optima_algorithm_name(o.algorithm)
              << std::setw(12) << iters << std::setw(16) << ctol << std::setw(13) << otol
              << optim::optima_problem_scaling_name(scaling) << "\n";
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
