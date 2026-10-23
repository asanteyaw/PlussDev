<!-- generated from the_same_problem_three_ways.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The same problem, three ways

[Back to Derivatives](../optima_07_derivatives.md)


The Rosenbrock function on a box, solved with a traced objective, a hand-written gradient and a forward-mode objective.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::optim;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }
optim::ParameterPack<pluss::Series> pair(double x, double y) { return optim::pack_scalars({{"x", scalar(x)}, {"y", scalar(y)}}); }

optim::ConstrainedProblem with_box(optim::ProblemBuilder b) { return b.lbs(pair(-2, -2)).ubs(pair(2, 2)).build(); }

int main() {
  // 1. Reverse-mode autodiff: write the function once on Trace values.
  auto traced = with_box(optim::problem().objective([](const optim::ParameterPack<optim::Trace>& p) -> optim::Trace {
    return 100.0 * optim::square(p.at("y") - optim::square(p.at("x"))) + optim::square(1.0 - p.at("x"));
  }));

  // 2. Analytic: return the value and a pack with the same blocks holding the gradient.
  auto analytic = with_box(optim::problem().objective_analytic([](const optim::ParameterPack<pluss::Series>& p) {
    const double x = p.at("x").item<double>(), y = p.at("y").item<double>();
    const double value = 100.0 * (y - x * x) * (y - x * x) + (1.0 - x) * (1.0 - x);
    const double gx = -400.0 * x * (y - x * x) - 2.0 * (1.0 - x);
    const double gy = 200.0 * (y - x * x);
    return std::pair{scalar(value), pair(gx, gy)};
  }));

  // 3. Forward mode: the N parameters arrive as an array of Fwd<N> values and the result is one Fwd<N>.
  using F = optim::Fwd<2>;
  auto forward = with_box(optim::problem().objective_analytic(optim::fwd_objective<2>([](const std::array<F, 2>& p) -> F {
    return 100.0 * optim::square(p[1] - optim::square(p[0])) + optim::square(1.0 - p[0]);
  })));

  struct Mode { const char* label; const optim::ConstrainedProblem* problem; };
  const Mode modes[] = {{"autodiff", &traced}, {"analytic", &analytic}, {"fwd_objective", &forward}};
  std::cout << std::left << std::setw(16) << "mode" << std::setw(8) << "exit" << std::setw(12) << "iterations"
            << std::setw(14) << "evaluations" << std::setw(14) << "objective" << "x, y\n";
  for (const auto& m : modes) {
    const auto r = optim::optima(*m.problem, pair(-1.2, 1.0));
    const auto x = r.params.at("x").item<double>();
    const auto y = r.params.at("y").item<double>();
    std::cout << std::left << std::setw(16) << m.label << std::setw(8) << static_cast<int>(r.exit_flag) << std::setw(12)
              << r.iteration << std::setw(14) << r.function_evaluations << std::scientific << std::setprecision(2)
              << std::setw(14) << r.evaluation.value.item<double>() << std::fixed << std::setprecision(6) << x << ", " << y
              << "\n";
  }
  return 0;
}
```

Output

```text
mode            exit    iterations  evaluations   objective     x, y
autodiff        1       35          45            5.38e-10      0.999977, 0.999954
analytic        1       35          45            5.38e-10      0.999977, 0.999954
fwd_objective   1       35          45            5.38e-10      0.999977, 0.999954
```
The three runs are identical to the printed digits: 35 iterations, 45 evaluations, an objective of 5.38e-10, at (0.999977, 0.999954). That is the point. The form changes what you write, not what the algorithm does. `analytic` returns a pair, the value and a pack with the same blocks as the parameters holding the gradient. `fwd_objective<N>` takes a function of an `std::array<Fwd<N>, N>`, with the parameters flattened in block order, and `N` must equal the total number of parameters (a mismatch is an error that names both numbers).
