<!-- generated from the_lsr1_optimizer.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The L-SR1 trust-region optimizer

[Back to Septima 4: The L-SR1 Trust-Region Optimizer](../septima_04_lsr1_trust_region.md)

Septima's third solver is a **trust-region** method with a limited-memory symmetric-rank-one (SR1) model of the curvature. Where L-BFGS builds a model that is always a bowl (positive curvature) and takes whatever step the model suggests, L-SR1 keeps a *radius* inside which it trusts its model: a step is accepted only if the objective falls about as much as the model predicted, and the radius grows when the model is reliable and shrinks when it is not. The SR1 update can represent negative curvature, which is what lets the method see a direction of descent at a saddle.

The page shows the method in four situations. A first run prints every field of the result for a quadratic and then compares all three optimizers on a ten-variable chain, where they differ in what they *achieve*, not only in how long they take. Saddles are where the model difference matters. The settings that matter (memory and the starting radius) are measured, with a trace of the trust region at work. Finally the block form takes several parameter blocks and a complex parameter.

## Program 1: A first run, and a comparison

The first part prints every field of the result for a quadratic. The second part solves the ten-variable chained Rosenbrock function with all three optimizers.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::subsp::math::optim;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  {
    auto quadratic = [](const pluss::Series& x) -> pluss::Series { return (x - 3.0).square().sum(); };
    const auto r = optim::l_sr1_trust_region(quadratic, vec({0.0, 0.0}));
    std::cout << std::boolalpha << "converged            " << r.converged << "\n"
              << "exit_flag            " << static_cast<int>(r.exit_flag) << "  (1 = Converged)\n"
              << "message              " << r.message << "\n"
              << "params               " << r.params.select(0, 0).item<double>() << ", " << r.params.select(0, 1).item<double>() << "\n"
              << "objective_value      " << r.objective_value << "\n"
              << std::scientific << std::setprecision(1) << "gradient_norm        " << r.gradient_norm << "  (the 2-norm of the gradient)\n"
              << "step_norm            " << r.step_norm << "\n"
              << "iteration            " << r.iteration << "\n"
              << "function_evaluations " << r.function_evaluations << "  (the trust-region step may be tried more than once)\n\n";
  }

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

  std::cout << std::left << std::setw(12) << "optimizer" << std::setw(14) << "exit" << std::setw(8) << "iters" << std::setw(8) << "evals" << std::setw(12) << "objective"
            << "gradient\n";
  {
    const auto r = optim::l_sr1_trust_region(chain, vec(start));
    std::cout << std::left << std::setw(12) << "L-SR1" << std::setw(14) << (r.converged ? "Converged" : "not converged") << std::setw(8) << r.iteration << std::setw(8)
              << r.function_evaluations << std::scientific << std::setprecision(1) << std::setw(12) << r.objective_value << r.gradient_norm << "\n";
  }
  {
    optim::SeptimaSolveOptions options;
    options.max_iterations = 2000;
    options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
    const auto r = optim::septima(chain, vec(start), options);
    std::cout << std::left << std::setw(12) << "L-BFGS" << std::setw(14) << (r.converged ? "Converged" : "NoProgress") << std::setw(8) << r.iteration << std::setw(8)
              << r.function_evaluations << std::scientific << std::setprecision(1) << std::setw(12) << r.objective_value << r.gradient_inf_norm << "\n";
  }
  {
    optim::SeptimaSolveOptions options;
    options.max_iterations = 5000;
    options.step.adam.learning_rate = 0.05;
    const auto r = optim::septima(chain, vec(start), options);
    std::cout << std::left << std::setw(12) << "Adam" << std::setw(14) << (r.converged ? "Converged" : "MaxIterations") << std::setw(8) << r.iteration << std::setw(8)
              << r.function_evaluations << std::scientific << std::setprecision(1) << std::setw(12) << r.objective_value << r.gradient_inf_norm << "\n";
  }
  return 0;
}
```

Output

```text
converged            true
exit_flag            1  (1 = Converged)
message              l_sr1_trust_region: converged (||g||_2 <= gradient_tolerance)
params               3, 3
objective_value      0
gradient_norm        0.0e+00  (the 2-norm of the gradient)
step_norm            0.0e+00
iteration            3
function_evaluations 4  (the trust-region step may be tried more than once)

optimizer   exit          iters   evals   objective   gradient
L-SR1       Converged     115     262     2.0e-20     6.8e-09
L-BFGS      Converged     108     108     5.3e-20     6.1e-09
Adam        Converged     4388    4388    2.1e-17     3.6e-09
```
On the quadratic the method converges in 3 iterations and 4 function evaluations. The evaluations exceed the iterations because a trust-region step may be tried more than once: a step the function does not reward is rejected, the radius shrinks, and the step is tried again. `gradient_norm` is the **2-norm** of the gradient, where Septima's `gradient_inf_norm` is the largest component; the default tolerance is 1e-6 on it.

On the chain, all three optimizers converge. L-SR1 takes 115 iterations and 262 evaluations to reach an objective of 2.0e-20 with a gradient of 6.8e-09. L-BFGS takes 108 iterations and 108 evaluations to reach 5.3e-20 and 6.1e-09: it uses one evaluation per iteration where L-SR1 uses a little over two, and on this problem it is as accurate. Adam, at a learning rate of 0.05, converges after 4388 iterations. L-SR1's advantage on a problem like this is not accuracy but robustness to the shape of the function, as the saddle below shows.

## Program 2: Saddles

`f(x, y) = x^2 - y^2 + y^4 / 4` curves up along `x` and down along `y`. Its origin is a saddle, and its minima are at `(0, ±sqrt(2))`, where `f = -1`. Each run starts near the line `y = 0`, where the gradient in `y` is small, so that the starting point is almost stationary.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::subsp::math::optim;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  const auto index_x = pluss::make_series(std::vector<double>{0}, {1}).to(pluss::ScalarType::Int64);
  const auto index_y = pluss::make_series(std::vector<double>{1}, {1}).to(pluss::ScalarType::Int64);
  auto saddle = [&](const pluss::Series& v) -> pluss::Series {
    const pluss::Series x = v.index_select(0, index_x), y = v.index_select(0, index_y);
    return (x.square() - y.square() + 0.25 * y.square().square()).sum();
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
0.001       0.001     Converged, -1.0e+00       NoProgress, -8.1e-06
0.5         1e-06     Converged, -1.0e+00       Converged, -1.0e+00
0.001       1e-06     Converged, -1.0e+00       Converged, -1.0e+00
0.5         1e-09     Converged, -4.0e-18       Converged, -1.0e+00
```
From a start with `y = 0.1` both methods find the minimum. As the start moves closer to the saddle, L-BFGS can stop improving beside it: with `(x, y) = (0.001, 0.001)` it ends with `NoProgress` at an objective of -8.1e-06, essentially the saddle's value of 0, because its model is a bowl and sees nothing to gain by leaving; from the other starts, including `y = 1e-6`, it escapes and finds -1. L-SR1 finds the minimum, -1, from every one of these starts, because its model has a direction of falling curvature and the trust-region step follows it.

The last row is the caution. At `y = 1e-9` the gradient is already below the tolerance, so both methods report `Converged` at a point whose objective is -4e-18, the saddle. The tests the solvers use are first-order tests: a gradient of zero identifies a stationary point, and a saddle is as stationary as a minimum. If a saddle could be a stationary point of your problem, perturb the start, run from several starts, and compare the objective values.

## Program 3: The settings that matter

The memory of the curvature model, and the starting size of the trust region, on the chain; then the first twelve iterations.

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

  std::cout << std::left << std::setw(22) << "setting" << std::setw(15) << "exit" << std::setw(8) << "iters" << std::setw(8) << "evals" << "objective\n";
  const auto row = [&](const std::string& label, const optim::Lsr1SolveOptions& options) {
    const auto r = optim::l_sr1_trust_region(chain, vec(start), options);
    std::cout << std::left << std::setw(22) << label << std::setw(15) << optim::lsr1_exit_flag_name(r.exit_flag) << std::setw(8) << r.iteration << std::setw(8) << r.function_evaluations
              << std::scientific << std::setprecision(1) << r.objective_value << "\n";
  };
  for (const std::int64_t memory : {1, 3, 10, 30}) {
    optim::Lsr1SolveOptions options;
    options.max_iterations = 1000;
    options.step.memory_size = memory;
    row("memory " + std::to_string(memory), options);
  }
  for (const double radius : {0.01, 1.0, 100.0}) {
    optim::Lsr1SolveOptions options;
    options.max_iterations = 1000;
    options.step.initial_radius = radius;
    std::ostringstream label;
    label << "initial radius " << radius;
    row(label.str(), options);
  }

  std::cout << "\nthe first twelve iterations\n";
  optim::Lsr1SolveOptions options;
  options.max_iterations = 12;
  options.progress_hook = [](const optim::Lsr1ProgressSnapshot& s) {
    std::cout << std::right << std::setw(4) << s.iteration << std::scientific << std::setprecision(3) << "  objective " << s.objective << "  gradient " << s.gradient_norm
              << std::fixed << std::setprecision(3) << "  radius " << s.trust_radius << "  step " << s.step_norm << "\n";
    return false;
  };
  optim::l_sr1_trust_region(chain, vec(start), options);
  return 0;
}
```

Output

```text
setting               exit           iters   evals   objective
memory 1              MaxIterations  1000    1256    1.9e+00
memory 3              MaxIterations  1000    1538    1.6e-01
memory 10             Converged      115     262     2.0e-20
memory 30             Converged      115     262     2.0e-20
initial radius 0.01   Converged      148     313     4.5e-18
initial radius 1      Converged      115     262     2.0e-20
initial radius 100    Converged      145     427     5.7e-17

the first twelve iterations
   1  objective 7.074e+02  gradient 8.151e+02  radius 1.000  step 1.000
   2  objective 3.064e+02  gradient 3.890e+02  radius 1.000  step 0.694
   3  objective 1.240e+02  gradient 1.864e+02  radius 1.000  step 0.691
   4  objective 1.046e+02  gradient 2.747e+02  radius 1.000  step 1.000
   5  objective 5.346e+01  gradient 1.432e+02  radius 1.000  step 0.280
   6  objective 5.290e+01  gradient 9.880e+01  radius 0.250  step 0.500
   7  objective 3.624e+01  gradient 7.876e+01  radius 0.500  step 0.250
   8  objective 1.508e+01  gradient 5.381e+01  radius 0.500  step 0.500
   9  objective 1.274e+01  gradient 4.060e+01  radius 0.500  step 0.500
  10  objective 1.034e+01  gradient 2.599e+01  radius 0.500  step 0.323
  11  objective 9.103e+00  gradient 1.452e+01  radius 0.500  step 0.125
  12  objective 8.484e+00  gradient 5.051e+00  radius 0.500  step 0.080
```
The memory matters more than for L-BFGS. With a memory of 1 or 3 the model is too poor to follow the valley: neither converges in 1000 iterations, ending at 1.9 and 0.16. With 10 it converges in 115 iterations. A memory larger than the number of variables is capped at the number of variables (more than n step vectors in n dimensions are linearly dependent and add nothing), so 30 gives exactly the same run as 10 on this ten-variable problem; the default of 10 is a sensible value, and more never helps once it reaches n. The initial radius is adapted by the method and matters little: 1 (the default) converges in 115 iterations, 100 in 145, and 0.01, which starts the method taking tiny steps, converges in 148 iterations: a far-too-small radius costs iterations while it grows, but does not end the run. A starting radius above `max_radius` simply starts at the cap.

The trace shows the trust region at work. The radius sits at 1 while the model predicts well; at iteration 6 the decrease in the objective is much smaller than predicted, the radius drops to 0.25 and then recovers to 0.5, and the steps follow it: lengths of 0.5, 0.25, 0.5. The objective falls from 707 to 8.5 in twelve iterations. A radius that keeps shrinking would mean the model is poor, which usually means the memory is too small.

## Program 4: Several blocks, and a complex parameter

The block form takes the objective as a function of a list of `Series` blocks and returns the blocks in their original shapes and dtypes along with the result.

```cpp
#include <pluss/ma.h>

#include <complex>
#include <iomanip>
#include <iostream>

namespace optim = pluss::subsp::math::optim;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  std::cout << std::fixed << std::setprecision(4);
  {
    const auto target = vec({1.0, 2.0, 3.0});
    auto objective = [&](const std::vector<pluss::Series>& blocks) -> pluss::Series {
      const pluss::Series& w = blocks[0];
      const pluss::Series& b = blocks[1];
      return (w - target).square().sum() + (b + 1.0).square() + (w.sum() - b).square();
    };
    const auto [params, r] = optim::l_sr1_trust_region(objective, std::vector<pluss::Series>{vec({0.0, 0.0, 0.0}), vec({0.0})});
    const auto w = params[0].host<double>();
    std::cout << "blocks: w = (" << w[0] << ", " << w[1] << ", " << w[2] << "), b = " << as_f64(params[1]) << "   "
              << (r.converged ? "Converged" : "not converged") << " in " << r.iteration << " iterations\n";
  }
  {
    using C = std::complex<double>;
    const auto target = pluss::make_series(std::vector<C>{{1.0, 2.0}}, {1});
    auto distance = [&](const pluss::Series& z) -> pluss::Series {
      const pluss::Series difference = z - target;
      return difference.abs().pow(2.0).sum();
    };
    const auto r = optim::l_sr1_trust_region(distance, pluss::make_series(std::vector<C>{{0.0, 0.0}}, {1}));
    const C z = r.params.host<C>()[0];
    std::cout << "complex: z = " << z.real() << " + " << z.imag() << "i   " << (r.converged ? "Converged" : "not converged") << " in " << r.iteration << " iterations\n";
  }
  return 0;
}
```

Output

```text
blocks: w = (-0.4000, 0.6000, 1.6000), b = 0.4000   Converged in 4 iterations
complex: z = 1.0000 + 2.0000i   Converged in 2 iterations
```
The first problem fits a vector `w` and a scalar `b`; the answer, `w = (-0.4, 0.6, 1.6)` and `b = 0.4`, satisfies the optimality conditions of the objective, and is found in 4 iterations. The second fits a complex number to the target `1 + 2i` with a real loss and finds it in 2. As with the other optimizers, the real and imaginary parts are treated as two independent coordinates; the loss must be real.

## Decisions and pitfalls

- **Evaluations exceed iterations in a trust-region method.** A step the function does not reward is rejected, the radius shrinks and the step is tried again; read evaluations, not iterations, when each is expensive.
- **L-SR1 costs more evaluations per iteration.** On the chain it needed 262 evaluations where L-BFGS needed 108, for an equally accurate answer; choose it for robustness near saddles, not for precision alone.
- **All three tests are first-order.** A zero gradient identifies a stationary point and a saddle is as stationary as a minimum; if a saddle could be an answer, check the second-order behaviour yourself.
- **L-BFGS stops beside a saddle; L-SR1 can leave it.** The bowl model of L-BFGS sees nothing to gain, while the SR1 model sees the negative curvature; but at a start already below the tolerance, neither moves.
- **Keep the memory at 10 or more.** A memory of 1 or 3 is too poor to follow the valley and neither converges in 1000 iterations; a memory above the number of variables is capped at it.
- **Watch the radius.** A radius that keeps collapsing means the model predicts the objective badly (nearly flat or non-smooth region); a radius that always sits at its maximum means you can afford larger steps.

Where next: [Chapter 5](../septima_05_choosing_and_worked_problems.md) chooses between the solvers on worked problems.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
