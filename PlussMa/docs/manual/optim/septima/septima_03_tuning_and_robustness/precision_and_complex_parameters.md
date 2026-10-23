<!-- generated from precision_and_complex_parameters.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Precision and complex parameters

[Back to Tuning and Robustness](../septima_03_tuning_and_robustness.md)


The Rosenbrock valley in double and in single precision, and a complex number fitted to a target.

```cpp
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
```

Output

```text
precision   exit          iters   objective   x, y
Float64     NoProgress    102     5.6e-11     1.00000, 0.99999
Float32     Converged     109     3.6e-15     1.00000, 1.00000

complex parameter, minimize |z - (1 + 2i)|^2 from z = 0
  Adam    Converged  iterations 213  z = 1.0000 + 2.0000i
  L-BFGS  Converged  iterations 3    z = 1.0000 + 2.0000i
```
In double precision L-BFGS ends with `NoProgress` at an objective of 5.6e-11; in single precision it ends with `Converged` at 3.6e-15. Single precision has fewer digits, but in this run the smaller gradients it computes happen to satisfy the tolerances sooner; do not read it as single precision being better. The point is that single precision works, and that its floor is the accuracy of the arithmetic.

A complex parameter needs a real loss. Here `|z - (1 + 2i)|^2` is written with `abs` and `pow`, and Septima treats the real and imaginary parts as two independent coordinates. Adam finds `1 + 2i` in 213 iterations and L-BFGS in 3. Bounds on a complex parameter are rejected, because complex numbers have no order.
