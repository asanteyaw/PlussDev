<!-- generated from saddles.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Saddles

[Back to The L-SR1 Trust-Region Optimizer](../septima_04_lsr1_trust_region.md)


`f(x, y) = x^2 - y^2 + y^4 / 4` curves up along `x` and down along `y`. Its origin is a saddle, and its minima are at `(0, ±sqrt(2))`, where `f = -1`. Each run starts near the line `y = 0`, where the gradient in `y` is small, so that the starting point is almost stationary.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::optim;
using optim::Trace;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  const auto index_x = pluss::make_series(std::vector<double>{0}, {1}).to(pluss::ScalarType::Int64);
  const auto index_y = pluss::make_series(std::vector<double>{1}, {1}).to(pluss::ScalarType::Int64);
  auto saddle = [&](const Trace& v) -> Trace {
    const Trace x = pluss::subsp::math::diff::index_select(v, 0, index_x), y = pluss::subsp::math::diff::index_select(v, 0, index_y);
    return optim::sum(optim::square(x) - optim::square(y) + 0.25 * optim::square(optim::square(y)));
  };

  std::cout << std::left << std::setw(12) << "x0" << std::setw(10) << "eps" << std::setw(26) << "L-SR1 (exit, objective)" << "L-BFGS (exit, objective)\n";
  struct Start { double x0, eps; };
  for (const Start s : {Start{0.5, 1e-1}, Start{1e-3, 1e-3}, Start{0.5, 1e-6}, Start{1e-3, 1e-6}, Start{0.5, 1e-9}}) {
    const auto sr1 = optim::l_sr1_trust_region(saddle, vec({s.x0, s.eps}));
    optim::SeptimaSolveOptions options;
    options.max_iterations = 2000;
    options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
    const auto lbfgs = optim::septima(saddle, vec({s.x0, s.eps}), options);
    std::ostringstream a, b;
    a << (sr1.converged ? "Converged" : "not converged") << ", " << std::scientific << std::setprecision(1) << sr1.objective_value;
    b << (lbfgs.converged ? "Converged" : "NoProgress") << ", " << std::scientific << std::setprecision(1) << lbfgs.objective_value;
    std::cout << std::left << std::setw(12) << std::defaultfloat << s.x0 << std::setw(10) << s.eps << std::setw(26) << a.str() << b.str() << "\n";
  }
  return 0;
}
```

Output

```text
x0          eps       L-SR1 (exit, objective)   L-BFGS (exit, objective)
0.5         0.1       Converged, -1.0e+00       Converged, -1.0e+00
0.001       0.001     Converged, -1.0e+00       NoProgress, -8.0e-06
0.5         1e-06     Converged, -1.0e+00       NoProgress, -1.6e-11
0.001       1e-06     Converged, -1.0e+00       NoProgress, -1.6e-11
0.5         1e-09     Converged, -4.0e-18       Converged, -1.6e-17
```
From a start with `y = 0.1` both methods find the minimum. As the start moves closer to the saddle, L-BFGS stops improving beside it: with `(x, y) = (0.001, 0.001)` it ends with `NoProgress` at an objective of -8.0e-06, and with `y = 1e-6` at -1.6e-11, both essentially at the saddle's value of 0. Its model is a bowl, so it sees nothing to gain by leaving. L-SR1 finds the minimum, -1, from every one of these starts, because its model has a direction of falling curvature and the trust-region step follows it.

The last row is the caution. At `y = 1e-9` the gradient is already below the tolerance, so both methods report `Converged` at a point whose objective is -4e-18, the saddle. The tests the solvers use are first-order tests: a gradient of zero identifies a stationary point, and a saddle is as stationary as a minimum. If a saddle could be a stationary point of your problem, perturb the start, run from several starts, and compare the objective values.
