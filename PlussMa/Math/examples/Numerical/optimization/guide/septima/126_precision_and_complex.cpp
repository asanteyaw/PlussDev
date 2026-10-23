// Precision and complex parameters. (1) The same Rosenbrock run in double and in single precision with L-BFGS: single precision
// has about seven digits, so its objective cannot go below roughly 1e-14 times the scale of the problem, but it reaches that
// floor, and here it even ends by satisfying the tolerances where double precision stalls just short of them. (2) A complex
// parameter: find the complex number z that minimizes |z - (1 + 2i)|^2. The loss is real, as it must be; Septima treats the real
// and imaginary parts as two independent coordinates, so Adam and L-BFGS work unchanged. (A bound on a complex
// parameter is rejected, because complex numbers have no order.)
#include <pluss/ma.h>

#include <complex>
#include <iomanip>
#include <iostream>

namespace optim = pluss::optim;
using optim::Trace;
using Pack = optim::ParameterPack<Trace>;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }

int main() {
  std::function<Trace(const Pack&)> rosenbrock = [](const Pack& p) {
    return 100.0 * optim::square(p.at("y") - optim::square(p.at("x"))) + optim::square(1.0 - p.at("x"));
  };
  std::cout << std::left << std::setw(12) << "precision" << std::setw(14) << "exit" << std::setw(8) << "iters" << std::setw(12) << "objective" << "x, y\n";
  for (const auto dtype : {f64, pluss::ScalarType::Float32}) {
    const auto start = optim::pack_scalars({{"x", scalar(-1.2).to(dtype)}, {"y", scalar(1.0).to(dtype)}});
    optim::SeptimaSolveOptions options;
    options.max_iterations = 3000;
    options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
    const auto r = optim::septima(rosenbrock, start, options);
    std::cout << std::left << std::setw(12) << (dtype == f64 ? "Float64" : "Float32") << std::setw(14) << (r.converged ? "Converged" : "NoProgress") << std::setw(8)
              << r.iteration << std::scientific << std::setprecision(1) << std::setw(12) << r.objective_value << std::fixed << std::setprecision(5)
              << r.params[0].item<double>() << ", " << r.params[1].item<double>() << "\n";
  }

  using C = std::complex<double>;
  const auto target = pluss::make_series(std::vector<C>{{1.0, 2.0}}, {1});
  auto distance = [&](const Trace& z) -> Trace {
    const Trace difference = z - optim::fixed(target);
    return optim::sum(pluss::subsp::math::diff::pow(pluss::subsp::math::diff::abs(difference), 2.0));
  };
  std::cout << "\ncomplex parameter, minimize |z - (1 + 2i)|^2 from z = 0\n";
  for (const bool lbfgs : {false, true}) {
    optim::SeptimaSolveOptions options;
    options.max_iterations = 600;
    options.gradient_tolerance = 1e-5;
    options.step_tolerance = 1e-5;
    options.step.adam.learning_rate = 0.1;
    if (lbfgs) options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
    const auto r = optim::septima(distance, pluss::make_series(std::vector<C>{{0.0, 0.0}}, {1}), options);
    const C z = r.params[0].to_vec<C>()[0];
    std::cout << "  " << std::left << std::setw(8) << (lbfgs ? "L-BFGS" : "Adam") << (r.converged ? "Converged" : "not converged") << "  iterations " << std::setw(5) << r.iteration
              << "z = " << std::fixed << std::setprecision(4) << z.real() << " + " << z.imag() << "i\n";
  }
  return 0;
}
