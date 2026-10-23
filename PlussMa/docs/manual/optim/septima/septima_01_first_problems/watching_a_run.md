<!-- generated from watching_a_run.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Watching a run

[Back to First Problems](../septima_01_first_problems.md)


The progress hook is called after every iteration with a snapshot (iteration, function evaluations, objective, gradient, stationarity, step, violation, message). It returns `true` to keep going and `false` to stop with `Stopped`. This program prints every 150th iteration of an Adam run on the Rosenbrock valley with a learning rate of 0.05 and records when the objective first fell below 0.01.

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
  optim::SeptimaSolveOptions options;
  options.step.adam.learning_rate = 0.05;
  options.max_iterations = 1500;
  std::int64_t first_below = 0;
  options.progress_hook = [&](const optim::SeptimaProgressSnapshot& s) {
    if (s.iteration % 150 == 0 || s.iteration == 1)
      std::cout << std::setw(6) << s.iteration << std::scientific << std::setprecision(3) << "  objective " << s.objective << "  gradient "
                << s.gradient_inf_norm << "  step " << s.step_inf_norm << "\n";
    if (first_below == 0 && s.objective < 0.01) first_below = s.iteration;
    return true;
  };
  const auto r = optim::septima(rosenbrock, optim::pack_scalars({{"x", scalar(-1.2)}, {"y", scalar(1.0)}}), options);
  std::cout << "the objective first fell below 0.01 at iteration " << first_below << "; the run used " << r.iteration << " iterations\n";
  return 0;
}
```

Output

```text
     1  objective 2.420e+01  gradient 2.156e+02  step 5.000e-02
   150  objective 3.181e+00  gradient 1.748e+00  step 4.609e-03
   300  objective 1.231e+00  gradient 2.193e+00  step 5.793e-03
   450  objective 2.439e-01  gradient 6.607e-01  step 2.388e-03
   600  objective 6.592e-02  gradient 2.527e-01  step 1.574e-03
   750  objective 1.941e-02  gradient 1.174e-01  step 9.795e-04
   900  objective 5.552e-03  gradient 5.773e-02  step 5.908e-04
  1050  objective 1.464e-03  gradient 2.829e-02  step 3.398e-04
  1200  objective 3.438e-04  gradient 1.337e-02  step 1.837e-04
  1350  objective 7.014e-05  gradient 5.962e-03  step 9.220e-05
  1500  objective 1.216e-05  gradient 2.468e-03  step 4.255e-05
the objective first fell below 0.01 at iteration 831; the run used 1500 iterations
```
The objective falls from 24.2 to 1.2e-05 over 1500 iterations, with the gradient falling from 216 to 2.5e-03 and the step shrinking as it goes. It first fell below 0.01 at iteration 831, which is the kind of thing a hook can act on: stop at a target value, log to a file, or adjust something outside the solver. A trace like this is also the quickest way to see whether a learning rate is too small (a slow, steady fall, as here), too large (the objective oscillating), or about right.
