// The line search of sequential QP judges a trial point by a merit function: objective plus a penalty weight
// times the constraint violation. A weight that is too small lets the iterates drift away from the constraints
// because violating them looks cheap; a weight that is too large makes every step look like a loss of
// feasibility and slows the objective. The solver raises the weight itself when the subproblem needs it, so
// `initial_penalty` is a starting guess. The table starts from an infeasible point on a curved-valley problem with weights across four
// orders of magnitude; the result is identical, because the adaptive rule dominates the starting guess. Treat
// `initial_penalty` as a setting you almost never need.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }
optim::ParameterPack<pluss::Series> start_point() { return optim::pack_scalars({{"x", scalar(-1.9)}, {"y", scalar(2.0)}}); }
// Rosenbrock's curved valley, restricted to the disc x^2 + y^2 <= 1.5 and to the hyperbola x y = 0.4.
auto objective = [](const Pack& p) {
  return 100.0 * (p.at("y") - p.at("x").square()).square() + (1.0 - p.at("x")).square();
};
auto constraints() {
  return optim::constraints()
      .nl_ineq([](const Pack& p) { return p.at("x").square() + p.at("y").square() - 1.5; })
      .nl_eq([](const Pack& p) { return p.at("x") * p.at("y") - 0.4; });
}

int main() {
  std::cout << std::left << std::setw(34) << "setting" << std::setw(6) << "flag" << std::setw(7) << "iters"
            << std::setw(7) << "evals" << std::setw(13) << "objective" << "violation\n";
  auto run = [&](const std::string& label, const optim::OptimaOptions& options, const auto& start, const auto& cons, const auto& f) {
    const auto r = optim::optima(f, start, options, cons);
    std::cout << std::left << std::setw(34) << label << std::setw(6) << static_cast<int>(r.exit_flag) << std::setw(7)
              << r.iteration << std::setw(7) << r.function_evaluations << std::fixed << std::setprecision(5)
              << std::setw(13) << r.evaluation.value.template item<double>() << std::scientific << std::setprecision(1)
              << r.evaluation.max_constraint_violation << std::defaultfloat << "\n";
  };
  for (double w : {0.1, 1.0, 10.0, 100.0, 1000.0}) {
    optim::OptimaOptions o;
    o.algorithm = optim::OptimaAlgorithm::SequentialQP;
    o.sequential_qp.initial_penalty = w;
    run("initial_penalty " + std::to_string(w).substr(0, 7), o, start_point(), constraints(), objective);
  }
  return 0;
}
