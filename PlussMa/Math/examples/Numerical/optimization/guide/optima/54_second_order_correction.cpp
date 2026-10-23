// The Maratos effect: near the solution of a curved equality constraint, the best step of the linearized model
// can raise both the objective and the violation of the true problem, so a line search rejects a step that was
// in fact good, and the iteration crawls. A second-order correction re-solves the subproblem with the constraint
// values measured at the trial point, which cancels the curvature error. The problem is the classic one: minimize
// 2(x^2 + y^2 - 1) - x on the unit circle, started on the circle. Its minimum is at (1, 0) with value -1.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <cmath>
#include <string>
#include <vector>

namespace optim = pluss::optim;
using Pack = optim::ParameterPack<optim::Trace>;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }
optim::ParameterPack<pluss::Series> four(double a, double b, double c, double d) {
  return optim::pack_scalars({{"a", scalar(a)}, {"b", scalar(b)}, {"c", scalar(c)}, {"d", scalar(d)}});
}

int main() {
  const auto start = optim::pack_scalars({{"x", scalar(std::cos(0.9))}, {"y", scalar(std::sin(0.9))}});
  auto f = [](const Pack& p) { return 2.0 * (optim::square(p.at("x")) + optim::square(p.at("y")) - 1.0) - p.at("x"); };
  const auto cons = optim::constraints().nl_eq([](const Pack& p) {
    return optim::square(p.at("x")) + optim::square(p.at("y")) - 1.0;
  });
  std::cout << std::left << std::setw(30) << "max_second_order_corrections" << std::setw(7) << "iters" << std::setw(7)
            << "evals" << std::setw(12) << "objective" << "x\n";
  for (std::int64_t k : {0, 1, 2}) {
    optim::OptimaOptions o;
    o.algorithm = optim::OptimaAlgorithm::SequentialQP;
    o.sequential_qp.max_second_order_corrections = k;
    const auto r = optim::optima(f, start, o, cons);
    std::cout << std::left << std::setw(30) << k << std::setw(7) << r.iteration << std::setw(7) << r.function_evaluations
              << std::fixed << std::setprecision(6) << std::setw(12) << r.evaluation.value.item<double>()
              << r.params.at("x").item<double>() << "\n";
  }
  return 0;
}
