<!-- generated from bounds.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Bounds

[Back to Constraints](../septima_02_constraints.md)


The target is `(2, 1, -1)` and the objective is half the squared distance to it, with the box `[0, 1.5]` on every coordinate. The answer clips each coordinate of the target into the box: `(1.5, 1, 0)`. The run starts outside the box, at `(3, -2, 0.5)`.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <sstream>

namespace optim = pluss::optim;
using optim::Trace;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  const auto target = vec({2.0, 1.0, -1.0});
  auto objective = [&](const Trace& x) -> Trace { return optim::sum(optim::square(x - optim::fixed(target))) * 0.5; };

  optim::SeptimaConstraints box;
  box.lbs = {vec({0.0, 0.0, 0.0})};
  box.ubs = {vec({1.5, 1.5, 1.5})};

  std::cout << std::left << std::setw(10) << "optimizer" << std::setw(11) << "exit" << std::setw(7) << "iters" << std::setw(24) << "x" << std::setw(11)
            << "gradient" << "stationarity\n";
  for (const auto optimizer : {optim::SeptimaOptimizer::Adam, optim::SeptimaOptimizer::Lbfgs}) {
    optim::SeptimaSolveOptions options;
    options.step.optimizer = optimizer;
    options.step.adam.learning_rate = 0.05;
    options.max_iterations = 2000;
    // Start outside the box: the first step already brings it back in.
    const auto r = optim::septima(objective, vec({3.0, -2.0, 0.5}), options, box);
    std::ostringstream x;
    x << std::fixed << std::setprecision(4) << r.params[0].select(0, 0).item<double>() << ", " << r.params[0].select(0, 1).item<double>() << ", "
      << r.params[0].select(0, 2).item<double>();
    std::cout << std::left << std::setw(10) << (optimizer == optim::SeptimaOptimizer::Adam ? "Adam" : "L-BFGS") << std::setw(11)
              << (r.converged ? "Converged" : "not converged") << std::setw(7) << r.iteration << std::setw(24) << x.str() << std::scientific
              << std::setprecision(1) << std::setw(11) << r.gradient_inf_norm << r.stationarity << "\n";
  }
  return 0;
}
```

Output

```text
optimizer exit       iters  x                       gradient   stationarity
Adam      Converged  292    1.5000, 1.0000, 0.0000  1.0e+00    2.8e-07
L-BFGS    Converged  2      1.5000, 1.0000, 0.0000  1.0e+00    0.0e+00
```
Both optimizers find `(1.5, 1, 0)`, Adam in 292 iterations and L-BFGS in 2. Look at the last two columns. The objective's own gradient is 1.0 at the answer, because the objective still pulls toward the target through the bounds on the first and third coordinates. The stationarity is 2.8e-07 for Adam and exactly 0 for L-BFGS, because the components that point out of the box are removed. This is why a run with active bounds converges, and why `gradient_inf_norm` alone would be the wrong thing to watch. Because every iterate is clipped, a bound never causes a violation, and the start outside the box is brought inside by the first step.
