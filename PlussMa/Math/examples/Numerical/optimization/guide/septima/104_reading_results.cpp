// Everything a Septima run returns. The problem is a bounded fit: minimize 0.5 * ((a - 2)^2 + (b - 1)^2) with a <= 1.5,
// so the bound on `a` is active at the answer. After the run the program prints each field of the result with a
// one-line reading, in the order they are most useful: did it work, what is the answer, how good is it, what did it cost.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::subsp::math::optim;
using pluss::Series;
using Pack = optim::ParameterPack<pluss::Series>;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }

int main() {
  std::function<pluss::Series(const Pack&)> objective = [](const Pack& p) {
    return 0.5 * ((p.at("a") - 2.0).square() + (p.at("b") - 1.0).square());
  };
  optim::SeptimaConstraints constraints;
  constraints.ubs = {scalar(1.5), scalar(100.0)};  // one bound per block, in block order

  optim::SeptimaSolveOptions options;
  options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
  const auto r = optim::septima(objective, optim::pack_scalars({{"a", scalar(0.0)}, {"b", scalar(0.0)}}), options, constraints);

  std::cout << std::scientific << std::setprecision(2) << std::boolalpha;
  std::cout << "converged               " << r.converged << "\n";
  std::cout << "exit_flag               " << static_cast<int>(r.exit_flag) << "  (1 = Converged)\n";
  std::cout << "message                 " << r.message << "\n";
  std::cout << std::fixed << std::setprecision(4);
  std::cout << "params                  a = " << as_f64(r.params[0]) << ", b = " << as_f64(r.params[1]) << "\n";
  std::cout << "objective_value         " << r.objective_value << "\n";
  std::cout << std::scientific << std::setprecision(2);
  std::cout << "gradient_inf_norm       " << r.gradient_inf_norm << "  (the objective's gradient alone)\n";
  std::cout << "stationarity            " << r.stationarity << "  (with the active bound taken into account)\n";
  std::cout << "step_inf_norm           " << r.step_inf_norm << "  (size of the last step)\n";
  std::cout << "constraint_violation    " << r.constraint_violation << "\n";
  std::cout << "iteration               " << r.iteration << "\n";
  std::cout << "function_evaluations    " << r.function_evaluations << "\n";
  return 0;
}
