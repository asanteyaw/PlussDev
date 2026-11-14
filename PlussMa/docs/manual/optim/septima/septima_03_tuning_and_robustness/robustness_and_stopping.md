<!-- generated from robustness_and_stopping.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Robustness, stopping rules and precision

[Back to Septima 3: Tuning and Robustness](../septima_03_tuning_and_robustness.md)

Real objectives are undefined in some region of the parameter space, real problems are badly scaled, and real runs need to know when to stop. This page covers the three things that decide whether a run is trustworthy. First, an objective such as `-x - 0.2 log(1 - x)` is defined only for `x < 1`, and a large first step can jump past it; the NaN-recovery settings retry an update with a smaller step, but they look only at non-finite *parameters*, not at where the objective becomes undefined, so the learning rate is the real control.

Second, the stopping rules: three gradient tolerances on the ten-variable chain show that asking for more than a method can deliver changes the flag and nothing else, and a problem whose curvature differs by 10 000 between its variables shows how differently Adam and L-BFGS cope with scale. Third, precision and complex parameters: the same valley in double and in single precision, and a complex number fitted to a target (with bounds on a complex parameter rejected, since complex numbers have no order).

## Program 4: An objective that is undefined in part of the space

`f(x) = -x - 0.2 log(1 - x)` is defined only for `x < 1` and has its minimum at `x = 0.8`. With a large learning rate the first Adam step from 0 jumps past 1, where the logarithm is NaN.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::subsp::math::optim;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  auto objective = [](const pluss::Series& x) -> pluss::Series { return (x * -1.0).sum() - 0.2 * (1.0 - x).log().sum(); };

  std::cout << std::left << std::setw(42) << "setup" << std::setw(22) << "exit" << std::setw(8) << "iters" << "x\n";
  const auto run = [&](const char* label, std::int64_t recovery_attempts, bool bounded) {
    optim::SeptimaSolveOptions options;
    options.max_iterations = 800;
    options.gradient_tolerance = 1e-5;
    options.step_tolerance = 1e-5;
    options.step.adam.learning_rate = 1.5;
    options.step.nan_recovery_attempts = recovery_attempts;
    optim::SeptimaConstraints constraints;
    if (bounded) constraints.ubs = {vec({0.99})};
    const auto r = optim::septima(objective, vec({0.0}), options, constraints);
    const char* exit = r.exit_flag == optim::SeptimaExitFlag::Converged ? "Converged"
                       : r.exit_flag == optim::SeptimaExitFlag::NonFiniteEvaluation ? "NonFiniteEvaluation" : "MaxIterations";
    std::cout << std::left << std::setw(42) << label << std::setw(22) << exit << std::setw(8) << r.iteration << std::fixed << std::setprecision(4)
              << as_f64(r.params[0]) << "\n";
  };
  run("learning rate 1.5, no recovery", 0, false);
  run("learning rate 1.5, 6 recovery attempts", 6, false);
  run("learning rate 1.5, bound x <= 0.99", 2, true);
  return 0;
}
```

Output

```text
setup                                     exit                  iters   x
learning rate 1.5, no recovery            NonFiniteEvaluation   2       1.5000
learning rate 1.5, 6 recovery attempts    NonFiniteEvaluation   2       1.5000
learning rate 1.5, bound x <= 0.99        Converged             422     0.8000
```
The first run ends after two iterations with `NonFiniteEvaluation`, at `x = 1.5`. The second asks for six recovery attempts and ends the same way. The settings `nan_recovery_attempts` and `nan_recovery_shrink` retry an update that itself produces non-finite parameters or optimizer state with a smaller step. They do not look at the objective at the new point, so they cannot rescue a step into a region where the objective is undefined. What does rescue it is a bound that keeps every iterate inside the domain: with `x <= 0.99` the same run converges to 0.8 in 422 iterations. A smaller learning rate would also avoid the jump here, but a bound makes the guarantee. The general rule is to **bound the parameters wherever the objective is undefined outside the bounds**, and to set the bound a little inside the edge of the domain.

## Program 5: What stops a run

Three gradient tolerances on the ten-variable chain with L-BFGS, and then a problem whose curvature differs by a factor of 10 000 between its two variables.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::subsp::math::optim;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  const int n = 10;
  std::vector<double> first, second, start;
  for (int i = 0; i + 1 < n; ++i) { first.push_back(i); second.push_back(i + 1); }
  for (int i = 0; i < n; ++i) start.push_back(i % 2 == 0 ? -1.2 : 1.0);
  const auto head = pluss::make_series(first, {n - 1}).to(pluss::ScalarType::Int64);
  const auto tail = pluss::make_series(second, {n - 1}).to(pluss::ScalarType::Int64);
  auto chain = [&](const pluss::Series& x) -> pluss::Series {
    const pluss::Series a = x.index_select(0, head), b = x.index_select(0, tail);
    return (100.0 * (b - a.square()).square() + (1.0 - a).square()).sum();
  };

  std::cout << std::left << std::setw(20) << "gradient tolerance" << std::setw(14) << "exit" << std::setw(8) << "iters" << std::setw(12) << "gradient" << "objective\n";
  for (const double tolerance : {1e-3, 1e-6, 1e-9}) {
    optim::SeptimaSolveOptions options;
    options.max_iterations = 2000;
    options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
    options.gradient_tolerance = tolerance;
    options.step_tolerance = 1e-8;
    const auto r = optim::septima(chain, vec(start), options);
    std::cout << std::left << std::setw(20) << std::scientific << std::setprecision(0) << tolerance << std::setw(14)
              << (r.converged ? "Converged" : "NoProgress") << std::setw(8) << r.iteration << std::setprecision(1) << std::setw(12) << r.gradient_inf_norm
              << r.objective_value << "\n";
  }

  std::cout << "\nconditioning: 0.5 * (x0^2 + 10000 x1^2) from (1, 1)\n";
  auto stretched = [](const pluss::Series& x) -> pluss::Series { return (vec({1.0, 1e4}) * x.square()).sum() * 0.5; };
  struct Row { const char* label; bool lbfgs; double rate; };
  for (const Row row : {Row{"Adam, learning rate 0.01", false, 0.01}, Row{"Adam, learning rate 0.1", false, 0.1}, Row{"L-BFGS", true, 1.0}}) {
    optim::SeptimaSolveOptions options;
    options.max_iterations = 5000;
    if (row.lbfgs) options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
    else options.step.adam.learning_rate = row.rate;
    const auto r = optim::septima(stretched, vec({1.0, 1.0}), options);
    std::cout << "  " << std::left << std::setw(26) << row.label << (r.converged ? "Converged" : "not converged") << "  iterations " << std::setw(6) << r.iteration
              << std::scientific << std::setprecision(1) << r.objective_value << "\n";
  }
  return 0;
}
```

Output

```text
gradient tolerance  exit          iters   gradient    objective
1e-03               Converged     105     1.5e-06     2.9e-15
1e-06               Converged     106     2.1e-07     4.7e-17
1e-09               Converged     111     1.9e-10     3.7e-23

conditioning: 0.5 * (x0^2 + 10000 x1^2) from (1, 1)
  Adam, learning rate 0.01  Converged  iterations 531   2.6e-17
  Adam, learning rate 0.1   Converged  iterations 422   2.0e-17
  L-BFGS                    Converged  iterations 5     6.2e-33
```
With a gradient tolerance of 1e-3 the run converges at iteration 105 with a gradient of 1.5e-06, and with 1e-6 only one iteration later, at 106 with 2.1e-07: the gradient of a quasi-Newton run falls steeply at the end, so a thousand-fold tighter tolerance costs almost nothing. Asking for 1e-9 costs 6 more iterations than 1e-3 and reaches a gradient of 1.9e-10 and an objective of 3.7e-23. **A tolerance sets where the run stops, so choose it from the accuracy you can use; below that accuracy, extra digits cost iterations and change nothing you will read.**

On the stretched quadratic, both Adam runs converge (in 531 and 422 iterations), and L-BFGS in 5. Adam divides each coordinate's step by the size of that coordinate's own gradients, so a stretch along the axes costs it little. L-BFGS learns the curvature itself and needs far fewer iterations.

## Program 6: Precision and complex parameters

The Rosenbrock valley in double and in single precision, and a complex number fitted to a target.

```cpp
#include <pluss/ma.h>

#include <complex>
#include <iomanip>
#include <iostream>

namespace optim = pluss::subsp::math::optim;
using pluss::Series;
using Pack = optim::ParameterPack<pluss::Series>;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }

int main() {
  std::function<pluss::Series(const Pack&)> rosenbrock = [](const Pack& p) {
    return 100.0 * (p.at("y") - p.at("x").square()).square() + (1.0 - p.at("x")).square();
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
              << as_f64(r.params[0]) << ", " << as_f64(r.params[1]) << "\n";
  }

  using C = std::complex<double>;
  const auto target = pluss::make_series(std::vector<C>{{1.0, 2.0}}, {1});
  auto distance = [&](const pluss::Series& z) -> pluss::Series {
    const pluss::Series difference = z - target;
    return difference.abs().pow(2.0).sum();
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
    const C z = r.params[0].host<C>()[0];
    std::cout << "  " << std::left << std::setw(8) << (lbfgs ? "L-BFGS" : "Adam") << (r.converged ? "Converged" : "not converged") << "  iterations " << std::setw(5) << r.iteration
              << "z = " << std::fixed << std::setprecision(4) << z.real() << " + " << z.imag() << "i\n";
  }
  return 0;
}
```

Output

```text
precision   exit          iters   objective   x, y
Float64     Converged     81      1.7e-24     1.00000, 1.00000
Float32     Converged     110     0.0e+00     1.00000, 1.00000

complex parameter, minimize |z - (1 + 2i)|^2 from z = 0
  Adam    Converged  iterations 213  z = 1.0000 + 2.0000i
  L-BFGS  Converged  iterations 3    z = 1.0000 + 2.0000i
```
Both precisions end with `Converged`: double precision at an objective of 1.7e-24 in 81 iterations, single precision at 0 in 110. Single precision has fewer digits, so it needs more iterations to get its gradient and step under the tolerances; the point is that single precision works, and that its floor is the accuracy of the arithmetic.

A complex parameter needs a real loss. Here `|z - (1 + 2i)|^2` is written with `abs` and `pow`, and Septima treats the real and imaginary parts as two independent coordinates. Adam finds `1 + 2i` in 213 iterations and L-BFGS in 3. Bounds on a complex parameter are rejected, because complex numbers have no order.

## Decisions and pitfalls

- **Control the step, not the recovery.** If a step can leave the domain, use a smaller learning rate; the recovery settings fix non-finite parameters, they do not know where your objective ends.
- **A tolerance is a stopping point, not an accuracy guarantee.** Tightening it from 1e-6 to 1e-9 here cost 3 iterations and bought a gradient of 1e-10; tighten only as far as you will use.
- **Adam copes with stretch, L-BFGS with curvature.** Adam divides each coordinate's step by its own gradient scale; L-BFGS learns the curvature and needs far fewer iterations when it can.
- **Single precision is not better, it is different.** It converged sooner in one run by chance; use it when memory or speed demands it and check the answer.
- **A complex parameter needs a real loss.** Write it with `abs` and `pow`; the real and imaginary parts are treated as independent coordinates.

Where next: [Chapter 4](../septima_04_lsr1_trust_region.md) introduces the third solver, a trust-region method.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tuning_adam_and_lbfgs.html">
    <span class="label">Previous</span>
    <span class="title">Tuning Adam and L-BFGS</span>
  </a>
</div>
<!-- apiref:nav:end -->
