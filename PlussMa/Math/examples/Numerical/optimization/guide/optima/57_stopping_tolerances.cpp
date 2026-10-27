// A run stops when the iterate is feasible to `constraint_tolerance` and stationary to `optimality_tolerance`
// (both judged relative to the problem's own scale), or when the step or the objective stops changing by more
// than `step_tolerance` or `value_progress_tolerance`. The sweep tightens the first pair together; the message
// says which test ended each run.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }
optim::ParameterPack<pluss::Series> four(double a, double b, double c, double d) {
  return optim::pack_scalars({{"a", scalar(a)}, {"b", scalar(b)}, {"c", scalar(c)}, {"d", scalar(d)}});
}
auto objective = [](const Pack& p) { return p.at("a") * p.at("d") * (p.at("a") + p.at("b") + p.at("c")) + p.at("c"); };
auto constraints() {
  return optim::constraints()
      .nl_ineq([](const Pack& p) { return 25.0 - p.at("a") * p.at("b") * p.at("c") * p.at("d"); })
      .nl_eq([](const Pack& p) {
        return p.at("a").square() + p.at("b").square() + p.at("c").square() + p.at("d").square() - 40.0;
      })
      .lbs(four(1, 1, 1, 1))
      .ubs(four(5, 5, 5, 5));
}

int main() {
  for (double tol : {1e-3, 1e-6, 1e-9}) {
    optim::OptimaOptions o;
    o.algorithm = optim::OptimaAlgorithm::SequentialQP;
    o.sequential_qp.optimality_tolerance = tol;
    o.sequential_qp.constraint_tolerance = tol;
    const auto r = optim::optima(objective, four(1, 5, 5, 1), o, constraints());
    std::cout << std::scientific << std::setprecision(0) << "tolerances " << tol << ": " << std::fixed << std::setprecision(8)
              << r.evaluation.value.template item<double>() << ", " << r.iteration << " iterations, \""
              << r.message.substr(0, r.message.find('\n')) << "\"\n";
  }
  return 0;
}
