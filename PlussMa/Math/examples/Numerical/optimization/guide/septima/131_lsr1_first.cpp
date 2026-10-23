// The third optimizer: limited-memory SR1 with a trust region, `l_sr1_trust_region`. It is a sibling of Septima's Adam and
// L-BFGS for unconstrained problems, and it takes the objective the same way, on `Trace` values. Where L-BFGS takes a step
// along a direction it chooses and then shortens it if needed, L-SR1 decides how far to trust a model of the function
// before it steps: it minimizes a quadratic model inside a ball (the trust region), and grows or shrinks the ball
// according to how well the model predicted the real decrease. The program prints every field of the result for a
// quadratic, and then compares the three optimizers on the ten-variable chained Rosenbrock function.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::optim;
using optim::Trace;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  {
    auto quadratic = [](const Trace& x) -> Trace { return optim::sum(optim::square(x - 3.0)); };
    const auto r = optim::l_sr1_trust_region(quadratic, vec({0.0, 0.0}));
    std::cout << std::boolalpha << "converged            " << r.converged << "\n"
              << "exit_flag            " << static_cast<int>(r.exit_flag) << "  (1 = Converged)\n"
              << "message              " << r.message << "\n"
              << "params               " << r.params.select(0, 0).item<double>() << ", " << r.params.select(0, 1).item<double>() << "\n"
              << "objective_value      " << r.objective_value << "\n"
              << std::scientific << std::setprecision(1) << "gradient_norm        " << r.gradient_norm << "  (the 2-norm of the gradient)\n"
              << "step_norm            " << r.step_norm << "\n"
              << "iteration            " << r.iteration << "\n"
              << "function_evaluations " << r.function_evaluations << "  (the trust-region step may be tried more than once)\n\n";
  }

  const int n = 10;
  std::vector<double> first, second, start;
  for (int i = 0; i + 1 < n; ++i) { first.push_back(i); second.push_back(i + 1); }
  for (int i = 0; i < n; ++i) start.push_back(i % 2 == 0 ? -1.2 : 1.0);
  const auto head = pluss::make_series(first, {n - 1}).to(pluss::ScalarType::Int64);
  const auto tail = pluss::make_series(second, {n - 1}).to(pluss::ScalarType::Int64);
  auto chain = [&](const Trace& x) -> Trace {
    const Trace a = pluss::subsp::math::diff::index_select(x, 0, head), b = pluss::subsp::math::diff::index_select(x, 0, tail);
    return optim::sum(100.0 * optim::square(b - optim::square(a)) + optim::square(1.0 - a));
  };

  std::cout << std::left << std::setw(12) << "optimizer" << std::setw(14) << "exit" << std::setw(8) << "iters" << std::setw(8) << "evals" << std::setw(12) << "objective"
            << "gradient\n";
  {
    const auto r = optim::l_sr1_trust_region(chain, vec(start));
    std::cout << std::left << std::setw(12) << "L-SR1" << std::setw(14) << (r.converged ? "Converged" : "not converged") << std::setw(8) << r.iteration << std::setw(8)
              << r.function_evaluations << std::scientific << std::setprecision(1) << std::setw(12) << r.objective_value << r.gradient_norm << "\n";
  }
  {
    optim::SeptimaSolveOptions options;
    options.max_iterations = 2000;
    options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
    const auto r = optim::septima(chain, vec(start), options);
    std::cout << std::left << std::setw(12) << "L-BFGS" << std::setw(14) << (r.converged ? "Converged" : "NoProgress") << std::setw(8) << r.iteration << std::setw(8)
              << r.function_evaluations << std::scientific << std::setprecision(1) << std::setw(12) << r.objective_value << r.gradient_inf_norm << "\n";
  }
  {
    optim::SeptimaSolveOptions options;
    options.max_iterations = 5000;
    options.step.adam.learning_rate = 0.05;
    const auto r = optim::septima(chain, vec(start), options);
    std::cout << std::left << std::setw(12) << "Adam" << std::setw(14) << (r.converged ? "Converged" : "MaxIterations") << std::setw(8) << r.iteration << std::setw(8)
              << r.function_evaluations << std::scientific << std::setprecision(1) << std::setw(12) << r.objective_value << r.gradient_inf_norm << "\n";
  }
  return 0;
}
