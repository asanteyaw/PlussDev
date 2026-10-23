<!-- generated from an_inequality_constraint.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# An inequality constraint

[Back to Constraints](../septima_02_constraints.md)


The target `(2, 1)` lies outside the unit disc, and the problem is to be as close to it as possible while staying inside: minimize `0.5 |x - (2, 1)|^2` subject to `x0^2 + x1^2 - 1 <= 0`. The answer is the point of the circle in the direction of the target, `(2, 1) / sqrt(5) = (0.8944, 0.4472)`, and the multiplier is `(sqrt(5) - 1) / 2 = 0.618`.

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace optim = pluss::optim;
using optim::Trace;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  const auto target = vec({2.0, 1.0});
  auto objective = [&](const Trace& x) -> Trace { return optim::sum(optim::square(x - optim::fixed(target))) * 0.5; };
  optim::SeptimaConstraints disc;
  disc.inequalities = [](const std::vector<Series>& x) { return std::vector<Series>{x[0].square().sum() - 1.0}; };

  std::cout << "exact answer (" << std::fixed << std::setprecision(4) << 2.0 / std::sqrt(5.0) << ", " << 1.0 / std::sqrt(5.0)
            << "), multiplier " << (std::sqrt(5.0) - 1.0) / 2.0 << "\n\n";
  std::cout << std::left << std::setw(34) << "run" << std::setw(8) << "iters" << std::setw(20) << "x" << std::setw(12) << "violation" << "multiplier\n";
  const auto row = [](const char* label, const optim::SeptimaResult& r) {
    std::ostringstream x;
    x << std::fixed << std::setprecision(4) << r.params[0].select(0, 0).item<double>() << ", " << r.params[0].select(0, 1).item<double>();
    std::cout << std::left << std::setw(34) << label << std::setw(8) << r.iteration << std::setw(20) << x.str() << std::scientific << std::setprecision(1)
              << std::setw(12) << r.constraint_violation << std::fixed << std::setprecision(4) << r.state.lambda_inequalities[0].item<double>() << "\n";
  };

  optim::SeptimaSolveOptions base;  // the library defaults for the constraint schedule
  base.max_iterations = 4000;
  base.gradient_tolerance = 1e-5;
  base.step_tolerance = 1e-5;
  base.step.adam.learning_rate = 0.1;

  row("Adam from (0, 0)", optim::septima(objective, vec({0.0, 0.0}), base, disc));
  row("Adam from (-2, 1.5)", optim::septima(objective, vec({-2.0, 1.5}), base, disc));

  optim::SeptimaSolveOptions lbfgs = base;
  lbfgs.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
  row("L-BFGS from (0, 0)", optim::septima(objective, vec({0.0, 0.0}), lbfgs, disc));
  row("L-BFGS from (-2, 1.5)", optim::septima(objective, vec({-2.0, 1.5}), lbfgs, disc));
  lbfgs.step.lbfgs.line_search = optim::SeptimaLbfgsLineSearch::StrongWolfe;
  row("L-BFGS, strong Wolfe, far start", optim::septima(objective, vec({-2.0, 1.5}), lbfgs, disc));
  return 0;
}
```

Output

```text
exact answer (0.8944, 0.4472), multiplier 0.6180

run                               iters   x                   violation   multiplier
Adam from (0, 0)                  295     0.8944, 0.4472      0.0e+00     0.6180
Adam from (-2, 1.5)               660     0.8944, 0.4472      0.0e+00     0.6180
L-BFGS from (0, 0)                19      0.8944, 0.4472      4.4e-07     0.6180
L-BFGS from (-2, 1.5)             24      0.8944, 0.4472      4.4e-07     0.6180
L-BFGS, strong Wolfe, far start   24      0.8944, 0.4472      4.4e-07     0.6180
```
Every run finds the exact answer and the exact multiplier, 0.6180, from a start inside the disc and from one far outside it. Adam needs 295 and 660 iterations; L-BFGS needs 19 and 24. From the far start the multiplier first grows because the constraint is badly violated, and then has to come back down to 0.618 as the iterate returns; the multiplier update lets it do both. With constraints present, L-BFGS always protects each step with a backtracking test on the augmented function, because the inequality term has a kink where the constraint becomes active, so the line-search setting makes no difference in the last two rows: they are identical.
