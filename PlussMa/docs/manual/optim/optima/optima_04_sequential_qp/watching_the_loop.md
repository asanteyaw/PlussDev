<!-- generated from watching_the_loop.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Watching the loop

[Back to Sequential QP in Depth](../optima_04_sequential_qp.md)


The progress hook receives one snapshot per iteration. The program prints the step taken, the penalty weight, the stationarity measure and the constraint violation on a four-variable problem with one nonlinear inequality and one nonlinear equality.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace optim = pluss::optim;
using Pack = optim::ParameterPack<optim::Trace>;

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
        return optim::square(p.at("a")) + optim::square(p.at("b")) + optim::square(p.at("c")) + optim::square(p.at("d")) - 40.0;
      })
      .lbs(four(1, 1, 1, 1))
      .ubs(four(5, 5, 5, 5));
}

int main() {
  optim::OptimaOptions options;
  options.algorithm = optim::OptimaAlgorithm::SequentialQP;
  std::cout << std::right << std::setw(5) << "iter" << std::setw(12) << "step" << std::setw(12) << "penalty"
            << std::setw(14) << "stationarity" << std::setw(12) << "violation" << std::setw(12) << "objective\n";
  options.progress_hook = [](const optim::OptimaProgressSnapshot& s) {
    if (s.stage != optim::OptimaProgressStage::Iteration) return false;
    std::cout << std::setw(5) << s.iteration << std::scientific << std::setprecision(2) << std::setw(12) << s.step_size
              << std::setw(12) << s.backend_parameter << std::setw(14) << s.stationarity << std::setw(12)
              << s.evaluation.max_constraint_violation << std::fixed << std::setprecision(5) << std::setw(12)
              << s.evaluation.value.item<double>() << "\n";
    return false;
  };
  const auto r = optim::optima(objective, four(1, 5, 5, 1), options, constraints());
  std::cout << "\nexit flag " << static_cast<int>(r.exit_flag) << ", " << r.iteration << " iterations, "
            << r.function_evaluations << " evaluations\n" << r.message << "\n";
  return 0;
}
```

Output

```text
 iter        step     penalty  stationarity   violation  objective
    1    1.00e+00    5.00e+00      9.37e-02    1.39e+00    16.06250
    2    1.00e+00    2.50e+00      3.32e-02    8.08e-02    16.96396
    3    1.00e+00    1.25e+00      3.36e-03    4.61e-04    17.01372
    4    1.00e+00    1.21e+00      1.84e-05    8.24e-08    17.01402
    4    1.00e+00    1.21e+00      4.49e-07    8.24e-08    17.01402

exit flag 1, 4 iterations, 5 evaluations
optimality and feasibility tolerances satisfied
```
Read the columns together. The violation falls from 1.39 to 8.2e-08 in four iterations while the objective settles at 17.01402. The penalty weight starts at 5.00 and falls to 1.21: the solver sets it from the size of the multipliers rather than keeping the starting guess. The full step (`1.00`) is accepted every time, which is the sign of a model that predicts the problem well. The last iteration appears twice, with stationarity 1.84e-05 and then 4.49e-07: the second line is the same point after the multiplier estimate has been refreshed, and it is that value which satisfies the tolerance.
