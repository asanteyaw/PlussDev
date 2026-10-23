<!-- generated from the_learning_rate.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The learning rate

[Back to Tuning and Robustness](../septima_03_tuning_and_robustness.md)


The Rosenbrock valley is solved for 2000 iterations with five learning rates.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::optim;
using Pack = optim::ParameterPack<optim::Trace>;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }

int main() {
  std::function<optim::Trace(const Pack&)> rosenbrock = [](const Pack& p) {
    return 100.0 * optim::square(p.at("y") - optim::square(p.at("x"))) + optim::square(1.0 - p.at("x"));
  };
  const auto start = optim::pack_scalars({{"x", scalar(-1.2)}, {"y", scalar(1.0)}});

  std::cout << std::left << std::setw(15) << "learning rate" << std::setw(15) << "exit" << std::setw(8) << "iters" << std::setw(12) << "objective"
            << "x\n";
  for (const double rate : {0.001, 0.01, 0.05, 0.2, 1.0}) {
    optim::SeptimaSolveOptions options;
    options.max_iterations = 2000;
    options.step.adam.learning_rate = rate;
    const auto r = optim::septima(rosenbrock, start, options);
    std::cout << std::left << std::setw(15) << std::defaultfloat << rate << std::setw(15) << (r.converged ? "Converged" : "MaxIterations")
              << std::setw(8) << r.iteration << std::scientific << std::setprecision(1) << std::setw(12) << r.objective_value << std::fixed
              << std::setprecision(4) << r.params[0].item<double>() << "\n";
  }
  return 0;
}
```

Output

```text
learning rate  exit           iters   objective   x
0.001          MaxIterations  2000    3.9e+00     -0.9863
0.01           MaxIterations  2000    4.6e-02     0.7850
0.05           MaxIterations  2000    8.7e-09     0.9999
0.2            Converged      1486    3.8e-18     1.0000
1              MaxIterations  2000    1.4e-01     0.8711
```
At 0.001 the run is still at an objective of 3.9 when the budget ends, and at 0.01 at 4.6e-02: the steps are too short to get down the valley. At 0.05 it reaches 8.7e-09. At 0.2 it converges, and in 1486 iterations, so the best of these is the largest that is not too large. At 1 the run ends at an objective of 0.14, worse than at 0.05: the steps are too long for the narrow valley. The picture is general: there is a range of learning rates that works, it spans about one order of magnitude, and both sides of it are visibly worse. Find it by trying a factor of three or ten at a time and watching the trace (chapter 1, Program 6), not by reasoning about the problem.
