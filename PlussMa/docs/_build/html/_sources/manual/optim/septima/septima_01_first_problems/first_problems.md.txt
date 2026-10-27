<!-- generated from first_problems.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# First problems with Septima

[Back to Septima 1: First Problems](../septima_01_first_problems.md)

Septima is the library's solver for problems with *many* parameters and objectives you can differentiate: first-order Adam, and limited-memory L-BFGS, with bounds, equality and inequality constraints handled by an augmented-Lagrangian scheme. Where Optima solves a quadratic program at every step and wants a small problem it can take to high precision, Septima takes cheap steps and scales to large parameter sets, complex parameters and unusual dtypes.

The first page covers calling it. The smallest problem is solved four ways, which exposes the single most important setting (Adam's learning rate, whose default is far too small for a problem started three units from its answer). The Rosenbrock valley then compares Adam with L-BFGS on a hard curved problem, and the same objective is handed over in three forms: a function of one `Series`, named blocks of a `ParameterPack`, and a function of a list of `Series`.

## Program 1: The smallest problem, and why the defaults are not enough

Minimize `sum((x - 3)^2)` over two parameters, started from the origin. The objective is a lambda on a `Series`, written with the ordinary operations `square()` and `sum()`. The same problem is solved four ways.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::subsp::math::optim;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series start() { return pluss::make_series(std::vector<double>{0.0, 0.0}, {2}).to(f64); }

int main() {
  auto objective = [](const pluss::Series& x) -> pluss::Series { return (x - 3.0).square().sum(); };

  struct Row { const char* label; optim::SeptimaSolveOptions options; };
  std::vector<Row> rows(4);
  rows[0] = {"Adam, defaults", {}};
  rows[1] = {"Adam, defaults, 5000 iterations", {}};
  rows[1].options.max_iterations = 5000;
  rows[2] = {"Adam, learning rate 0.1", {}};
  rows[2].options.step.adam.learning_rate = 0.1;
  rows[3] = {"L-BFGS, defaults", {}};
  rows[3].options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;

  std::cout << std::left << std::setw(34) << "setup" << std::setw(16) << "exit" << std::setw(8) << "iters" << std::setw(14)
            << "objective" << "largest gradient\n";
  for (const auto& row : rows) {
    const auto r = optim::septima(objective, start(), row.options);
    std::cout << std::left << std::setw(34) << row.label << std::setw(16) << optim::septima_exit_flag_name(r.exit_flag) << std::setw(8) << r.iteration
              << std::scientific << std::setprecision(2) << std::setw(14) << r.objective_value << r.gradient_inf_norm << "\n";
  }
  return 0;
}
```

Output

```text
setup                             exit            iters   objective     largest gradient
Adam, defaults                    MaxIterations   200     1.57e+01      5.61e+00
Adam, defaults, 5000 iterations   MaxIterations   5000    7.78e-03      1.25e-01
Adam, learning rate 0.1           MaxIterations   200     8.93e-09      1.34e-04
L-BFGS, defaults                  Converged       3       0.00e+00      0.00e+00
```
Adam with its default settings uses all 200 iterations and stops with an objective of 15.7, far from the answer of 0: its default learning rate is 0.001, so each step moves a parameter by about a thousandth, and 200 steps cannot cross a distance of 3. Given 5000 iterations it reaches 7.8e-3. With a learning rate of 0.1 it gets to 8.9e-9 within the same 200 iterations, but the exit flag is still `MaxIterations`: a run counts as converged only when the largest gradient component is at most `gradient_tolerance` (1e-6), the last step is at most `step_tolerance` (1e-10), and the constraint violation is at most `constraint_tolerance` (1e-6). The gradient here is 1.3e-4, so the run is a good answer that has not yet met its own tests. L-BFGS needs three iterations and reports `Converged`, because on a quadratic it finds the exact step once it has seen the curvature.

The lesson is about the learning rate. It sets the size of every Adam step, so too small wastes the whole budget and too large overshoots. Chapter 3 shows how to choose it.

## Program 2: A curved valley

The Rosenbrock function `(1 - x)^2 + 100 (y - x^2)^2` has its minimum at `(1, 1)` at the end of a long, curved, narrow valley, and is the standard test of an optimizer. Here the parameters are two named scalar blocks, `x` and `y`, looked up by name inside the objective. The table gives iterations, function evaluations (a line search spends several per iteration), and the objective and point reached.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }

int main() {
  std::function<pluss::Series(const Pack&)> rosenbrock = [](const Pack& p) {
    return 100.0 * (p.at("y") - p.at("x").square()).square() + (1.0 - p.at("x")).square();
  };
  const auto start = optim::pack_scalars({{"x", scalar(-1.2)}, {"y", scalar(1.0)}});

  struct Row { const char* label; optim::SeptimaSolveOptions options; };
  std::vector<Row> rows(3);
  rows[0] = {"Adam, learning rate 0.02", {}};
  rows[0].options.step.adam.learning_rate = 0.02;
  rows[1] = {"L-BFGS, no line search", {}};
  rows[1].options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
  rows[2] = {"L-BFGS, strong Wolfe", {}};
  rows[2].options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
  rows[2].options.step.lbfgs.line_search = optim::SeptimaLbfgsLineSearch::StrongWolfe;
  for (auto& row : rows) row.options.max_iterations = 3000;

  std::cout << std::left << std::setw(28) << "setup" << std::setw(14) << "exit" << std::setw(8) << "iters" << std::setw(8) << "evals"
            << std::setw(12) << "objective" << "x, y\n";
  for (const auto& row : rows) {
    const auto r = optim::septima(rosenbrock, start, row.options);
    std::cout << std::left << std::setw(28) << row.label << std::setw(14) << optim::septima_exit_flag_name(r.exit_flag) << std::setw(8) << r.iteration
              << std::setw(8) << r.function_evaluations << std::scientific << std::setprecision(1) << std::setw(12) << r.objective_value
              << std::fixed << std::setprecision(5) << as_f64(r.params[0]) << ", " << as_f64(r.params[1]) << "\n";
  }
  return 0;
}
```

Output

```text
setup                       exit          iters   evals   objective   x, y
Adam, learning rate 0.02    MaxIterations 3000    3000    2.9e-07     0.99947, 0.99893
L-BFGS, no line search      NoProgress    102     102     5.6e-11     1.00000, 0.99999
L-BFGS, strong Wolfe        NoProgress    696     2062    3.1e-10     0.99999, 0.99998
```
Adam with a learning rate of 0.02 uses all 3000 iterations and ends at an objective of 2.9e-07, at `(0.99947, 0.99893)`: close, and still creeping. L-BFGS without a line search reaches 5.6e-11 at `(1.00000, 0.99999)` in 102 iterations. With the strong-Wolfe line search it takes 696 iterations and 2062 evaluations, about three evaluations per iteration, and ends at 3.1e-10: on this problem the line search buys nothing, and chapter 3 says when it does.

Two things to notice. Both L-BFGS runs end with `NoProgress`, not `Converged`, at an objective of about 1e-10. That flag means the objective and the step stopped changing before all three tolerances were met; here the gradient was a few times 1e-4 against a tolerance of 1e-6. It is not a failure. When a run ends this way, look at the objective and the gradient it reports and decide whether the answer is good enough, instead of reading the flag alone. And the iteration counts differ by a factor of thirty, which is the usual gap between a first-order step and one that uses curvature.

## Program 3: Three ways to hand over the same objective

Septima takes the objective in the form that suits your model. A single block of parameters is a `Series` and the objective is a function of one `Series`. A model with several parts uses a `ParameterPack` whose blocks have names, and the objective reads them with `p.at("name")`. With several blocks the objective can also be a function of a list of `Series`; the library differentiates it the same way. The program minimizes `(a - 1)^2 + (b + 2)^2` all three ways and then once more in single precision.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::subsp::math::optim;
using pluss::Series;
using Pack = optim::ParameterPack<pluss::Series>;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  optim::SeptimaSolveOptions options;
  options.step.adam.learning_rate = 0.1;
  options.max_iterations = 1000;
  options.gradient_tolerance = 1e-4;  // looser than the default, so that each run can end by converging
  options.step_tolerance = 1e-4;

  std::cout << std::fixed << std::setprecision(4) << std::left;
  const auto report = [](const char* label, double a, double b, const optim::SeptimaResult& r) {
    std::cout << std::setw(26) << label << "a = " << a << ", b = " << b << "   iterations " << r.iteration
              << (r.converged ? "   converged" : "   did not converge") << "\n";
  };

  // 1. One block: a vector (a, b).
  const auto one = optim::septima(
      [](const pluss::Series& v) -> pluss::Series {
        const pluss::Series target = vec({1.0, -2.0});
        return (v - target).square().sum();
      },
      vec({0.0, 0.0}), options);
  report("one block", one.params[0].select(0, 0).item<double>(), one.params[0].select(0, 1).item<double>(), one);

  // 2. Named blocks.
  std::function<pluss::Series(const Pack&)> named = [](const Pack& p) {
    return (p.at("a") - 1.0).square() + (p.at("b") + 2.0).square();
  };
  const auto two = optim::septima(named, optim::pack_scalars({{"a", scalar(0.0)}, {"b", scalar(0.0)}}), options);
  report("named blocks", as_f64(two.params[0]), as_f64(two.params[1]), two);

  // 3. A Series objective over a list of blocks.
  std::function<pluss::Series(const std::vector<pluss::Series>&)> plain = [](const std::vector<pluss::Series>& p) {
    return (p[0] - 1.0).square() + (p[1] + 2.0).square();
  };
  const auto three = optim::septima(plain, std::vector<pluss::Series>{scalar(0.0), scalar(0.0)}, options);
  report("Series objective", as_f64(three.params[0]), as_f64(three.params[1]), three);

  // The first form again, in single precision.
  const auto single = optim::septima(
      [](const pluss::Series& v) -> pluss::Series {
        const pluss::Series target = vec({1.0, -2.0}).to(pluss::ScalarType::Float32);
        return (v - target).square().sum();
      },
      vec({0.0, 0.0}).to(pluss::ScalarType::Float32), options);
  report("one block, Float32", single.params[0].select(0, 0).item<double>(), single.params[0].select(0, 1).item<double>(), single);
  return 0;
}
```

Output

```text
one block                 a = 1.0000, b = -2.0000   iterations 191   converged
named blocks              a = 1.0000, b = -2.0000   iterations 191   converged
Series objective          a = 1.0000, b = -2.0000   iterations 191   converged
one block, Float32        a = 1.0000, b = -2.0000   iterations 191   converged
```
The three forms find the same point, `a = 1`, `b = -2`, in the same 191 iterations, because they describe one function: the form changes what you write, not what the solver does. Single precision does too, to four digits. Use named blocks when the parameters mean different things (weights, biases, scales), because the objective then reads like the model; use one vector block when the parameters are interchangeable. Parameters can be real or complex, in single or double precision, on the CPU or on a device; with constraints (chapter 2) they must all be on one platform.

## Decisions and pitfalls

- **The learning rate is the setting that matters most for Adam.** Too small wastes the whole budget (a default of 0.001 cannot cross a distance of 3 in 200 steps); too large overshoots. Chapter 3 shows how to choose it.
- **Prefer L-BFGS when the objective is smooth.** It learns the curvature itself and needs far fewer iterations than Adam on a curved valley; Adam divides each coordinate's step by that coordinate's own gradient scale, which is its strength on noisy or badly scaled objectives.
- **`NoProgress` at an objective of 1e-10 is not a failure.** It means the objective and the step stopped changing before all three tolerances were met; look at the gradient before deciding.
- **Use named blocks when the parameters mean different things.** Weights, biases and scales are clearer by name, and the three forms of the objective find the same point: the form changes what you write, not what the solver does.

Where next: [Reading a run](reading_a_run.md) covers the result object, the exit flags and the progress hook.
