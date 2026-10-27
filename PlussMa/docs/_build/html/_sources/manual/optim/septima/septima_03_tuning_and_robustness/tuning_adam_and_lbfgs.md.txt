<!-- generated from tuning_adam_and_lbfgs.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Tuning Adam and L-BFGS

[Back to Septima 3: Tuning and Robustness](../septima_03_tuning_and_robustness.md)

Both solvers have a handful of settings that matter and many that do not. This page measures the ones that do, changing each on its own on a problem where its effect can be seen. For Adam the main setting is the **learning rate**, swept over five values on the Rosenbrock valley: too small leaves the run short of the answer when the budget ends, too large overshoots, and the best is the largest that is not too large. The other Adam settings are weight decay (which *changes the answer*, because it adds a term to the gradient), the momentum settings (which matter less than the learning rate and can hurt) and `maximize`.

For L-BFGS the settings are the history size (how many recent step pairs the method remembers), the line search (fewer iterations at the cost of more evaluations) and the length of the first trial step. A ten-variable chained Rosenbrock function, where every neighbouring pair of variables forms a valley, is the test.

## Program 1: The learning rate

The Rosenbrock valley is solved for 2000 iterations with five learning rates.

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

  std::cout << std::left << std::setw(15) << "learning rate" << std::setw(15) << "exit" << std::setw(8) << "iters" << std::setw(12) << "objective"
            << "x\n";
  for (const double rate : {0.001, 0.01, 0.05, 0.2, 1.0}) {
    optim::SeptimaSolveOptions options;
    options.max_iterations = 2000;
    options.step.adam.learning_rate = rate;
    const auto r = optim::septima(rosenbrock, start, options);
    std::cout << std::left << std::setw(15) << std::defaultfloat << rate << std::setw(15) << (r.converged ? "Converged" : "MaxIterations")
              << std::setw(8) << r.iteration << std::scientific << std::setprecision(1) << std::setw(12) << r.objective_value << std::fixed
              << std::setprecision(4) << as_f64(r.params[0]) << "\n";
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

## Program 2: The other Adam settings

Each setting changed on its own.

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
  std::cout << std::fixed << std::setprecision(4);
  {
    auto quadratic = [](const pluss::Series& x) -> pluss::Series { return (x - 3.0).square().sum(); };
    std::cout << "weight decay of 0.1 (the exact minimizer without decay is 3)\n";
    struct Row { const char* label; double decay; bool decoupled; };
    for (const Row row : {Row{"no decay", 0.0, false}, Row{"coupled decay", 0.1, false}, Row{"decoupled decay", 0.1, true}}) {
      optim::SeptimaSolveOptions options;
      options.max_iterations = 3000;
      options.gradient_tolerance = 1e-5;
      options.step_tolerance = 1e-5;
      options.step.adam.learning_rate = 0.05;
      options.step.adam.weight_decay = row.decay;
      options.step.adam.decoupled_weight_decay = row.decoupled;
      const auto r = optim::septima(quadratic, vec({0.0, 0.0}), options);
      const char* ended = r.converged ? "Converged" : r.exit_flag == optim::SeptimaExitFlag::NoProgress ? "NoProgress" : "MaxIterations";
      std::cout << "  " << std::left << std::setw(17) << row.label << std::setw(15) << ended << "iterations " << std::setw(6) << r.iteration
                << "x = " << r.params[0].select(0, 0).item<double>() << "\n";
    }
  }
  {
    std::function<pluss::Series(const Pack&)> rosenbrock = [](const Pack& p) {
      return 100.0 * (p.at("y") - p.at("x").square()).square() + (1.0 - p.at("x")).square();
    };
    const auto start = optim::pack_scalars({{"x", scalar(-1.2)}, {"y", scalar(1.0)}});
    std::cout << "\nRosenbrock, learning rate 0.05, 2000 iterations: objective reached\n";
    struct Row { const char* label; bool amsgrad; double beta1; double beta2; };
    for (const Row row : {Row{"defaults (beta1 0.9, beta2 0.999)", false, 0.9, 0.999}, Row{"amsgrad", true, 0.9, 0.999},
                          Row{"beta1 = 0.5", false, 0.5, 0.999}, Row{"beta2 = 0.9", false, 0.9, 0.9}}) {
      optim::SeptimaSolveOptions options;
      options.max_iterations = 2000;
      options.step.adam.learning_rate = 0.05;
      options.step.adam.amsgrad = row.amsgrad;
      options.step.adam.beta1 = row.beta1;
      options.step.adam.beta2 = row.beta2;
      const auto r = optim::septima(rosenbrock, start, options);
      std::cout << "  " << std::left << std::setw(36) << row.label << std::scientific << std::setprecision(2) << r.objective_value << std::fixed
                << std::setprecision(4) << "\n";
    }
  }
  {
    auto concave = [](const pluss::Series& x) -> pluss::Series { return x.square().sum() * -1.0 + x.sum() * 2.0; };
    optim::SeptimaSolveOptions options;
    options.max_iterations = 500;
    options.step.adam.learning_rate = 0.1;
    options.step.adam.maximize = true;
    const auto r = optim::septima(concave, vec({0.0}), options);
    std::cout << "\nmaximize: x = " << as_f64(r.params[0]) << ", objective = " << r.objective_value << "\n";
  }
  return 0;
}
```

Output

```text
weight decay of 0.1 (the exact minimizer without decay is 3)
  no decay         Converged      iterations 245   x = 3.0000
  coupled decay    NoProgress     iterations 544   x = 2.8571
  decoupled decay  MaxIterations  iterations 3000  x = 2.9594

Rosenbrock, learning rate 0.05, 2000 iterations: objective reached
  defaults (beta1 0.9, beta2 0.999)   8.69e-09
  amsgrad                             2.28e-05
  beta1 = 0.5                         4.28e-03
  beta2 = 0.9                         3.44e-03

maximize: x = 1.0000, objective = 1.0000
```
**Weight decay changes the answer.** Without decay the quadratic's minimum, 3, is found in 245 iterations. Coupled decay of 0.1 adds `0.1 x` to the gradient, so the solver is now minimizing a different function, whose minimum is `6 / 2.1 = 2.8571`, and it finds exactly that, ending with `NoProgress` because that is where it settles. Decoupled decay shrinks the parameter directly instead and, with the same value, is still moving after 3000 iterations at 2.9594. Weight decay is a regularizer: use it when you want the pull toward zero, and never to speed a run up.

**The momentum settings matter less than the learning rate and can hurt.** On the Rosenbrock valley at a learning rate of 0.05 the defaults reach 8.7e-09 after 2000 iterations. `amsgrad` reaches 2.3e-05, a shorter memory of the gradient (`beta1 = 0.5`) 4.3e-03, and a shorter memory of the squared gradient (`beta2 = 0.9`) 3.4e-03. Change them only with a reason, and one at a time.

**`maximize` flips the direction.** The last line maximizes `-x^2 + 2x` and finds `x = 1`, objective 1. The reported `objective_value` is the function you wrote, not its negative.

## Program 3: The L-BFGS settings

A ten-variable chained Rosenbrock function, where every neighbouring pair of variables forms a valley.

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

  std::cout << std::left << std::setw(32) << "setup" << std::setw(14) << "exit" << std::setw(8) << "iters" << std::setw(8) << "evals" << "objective\n";
  for (const bool wolfe : {false, true}) {
    for (const std::int64_t history : {1, 3, 10, 30}) {
      optim::SeptimaSolveOptions options;
      options.max_iterations = 2000;
      options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
      options.step.lbfgs.history_size = history;
      if (wolfe) options.step.lbfgs.line_search = optim::SeptimaLbfgsLineSearch::StrongWolfe;
      const auto r = optim::septima(chain, vec(start), options);
      std::cout << std::left << std::setw(32) << ((wolfe ? "strong Wolfe, history " : "no line search, history ") + std::to_string(history))
                << std::setw(14) << optim::septima_exit_flag_name(r.exit_flag) << std::setw(8) << r.iteration << std::setw(8) << r.function_evaluations << std::scientific
                << std::setprecision(1) << r.objective_value << "\n";
    }
  }
  std::cout << "\nlearning rate, no line search, default history (20)\n";
  for (const double rate : {0.1, 0.5, 1.0, 2.0}) {
    optim::SeptimaSolveOptions options;
    options.max_iterations = 2000;
    options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
    options.step.lbfgs.learning_rate = rate;
    const auto r = optim::septima(chain, vec(start), options);
    std::cout << "  " << std::left << std::setw(8) << std::defaultfloat << rate << std::setw(22) << optim::septima_exit_flag_name(r.exit_flag) << "iterations " << std::setw(6) << r.iteration
              << std::scientific << std::setprecision(1) << r.objective_value << "\n";
  }
  return 0;
}
```

Output

```text
setup                           exit          iters   evals   objective
no line search, history 1       NoProgress    214     214     1.4e-08
no line search, history 3       NoProgress    155     155     6.7e-09
no line search, history 10      NoProgress    137     137     9.5e-10
no line search, history 30      NoProgress    144     144     6.3e-11
strong Wolfe, history 1         NoProgress    176     508     2.3e-09
strong Wolfe, history 3         NoProgress    97      259     7.2e-09
strong Wolfe, history 10        NoProgress    94      248     7.8e-11
strong Wolfe, history 30        NoProgress    89      231     1.0e-10

learning rate, no line search, default history (20)
  0.1     NoProgress            iterations 413   4.6e-10
  0.5     NoProgress            iterations 150   1.5e-10
  1       NoProgress            iterations 124   3.3e-10
  2       NonFiniteEvaluation   iterations 32    inf
```
The history size matters at the short end. With a history of 1 the method uses only the last step pair and needs 214 iterations; with 3 it needs 155, with 10 it needs 137, and beyond that there is no gain (144 at a history of 30). The default of 20 is comfortably in the flat part.

(With equality or inequality constraints, chapter 2, L-BFGS always protects each step with a backtracking test on the augmented function, whatever this setting says.) The line search buys fewer iterations and costs more evaluations. With a history of 10, no line search needs 137 iterations and 137 evaluations; the strong-Wolfe search needs 94 iterations but 248 evaluations. When an evaluation is expensive, the run that uses fewer evaluations is the one without the line search. The line search earns its cost when a full step is not safe: with constraints (chapter 2), or on an objective with sharp changes in curvature.

The learning rate is the length of the first trial step. Values from 0.1 to 1 all work, and 1 needs the fewest iterations (124), 0.1 the most (413); at 2 the steps become so long that the run reaches non-finite values and ends with `NonFiniteEvaluation` after 32 iterations. Without a line search, keep it at or below 1.

All the L-BFGS runs here end with `NoProgress` at an objective near 1e-10. As in chapter 1, that is the method reaching the limit of what it can do with these tolerances, at a very good point.

## Decisions and pitfalls

- **Tune the learning rate first and alone.** It is the setting with the largest effect on Adam; try a sweep over powers of ten before touching anything else.
- **Weight decay changes the problem.** With coupled decay the solver minimises a different function; the answer moves from 3 to 6/2.1 on the example quadratic. Use it deliberately.
- **Leave the momentum settings at their defaults unless you can show a gain.** On the valley the defaults beat `amsgrad` and shorter memories.
- **An L-BFGS history of 10 to 20 is in the flat part.** History 1 needs far more iterations; beyond 10 there is no gain.
- **Without a line search keep the learning rate at or below 1.** At 2 the steps run into non-finite values and the run ends with `NonFiniteEvaluation`.
- **The line search trades iterations for evaluations.** Choose by which is expensive in your objective.

Where next: [Robustness and stopping](robustness_and_stopping.md) covers objectives that are undefined in places, stopping rules and precision.
