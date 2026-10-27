<!-- generated from curvature_scaling_and_size.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Curvature, scaling and size

[Back to Optima 3: Choosing and Tuning an Algorithm](../optima_03_choosing_an_algorithm.md)

Three properties of a problem decide how hard a given algorithm finds it, and they are the three things to check when a run is slow. The first is the **curvature model**: the quasi-Newton algorithms build a model of the second derivatives from the steps they take, and `hessian_approximation` chooses it. The second is **scaling**: a problem whose variables or constraints are written in very different units is the classic stress test for step-based methods, and `problem_scaling` rescales a copy of the problem before solving it. The third is **size**: the cost of a method shows in how its work grows with the number of variables.

The programs make each of these concrete: a chain with strongly coupled curvature solved with every curvature model, a problem written in units that differ by factors of 10⁸, and a smoothing fit grown from a few to many variables, with the iteration and evaluation counts side by side.

## Program 5: The curvature model

The quasi-Newton algorithms build a model of the curvature from the steps they take, and `hessian_approximation` chooses the model. Here a six-variable chain with strongly coupled curvature is minimized under bounds, with each model the two algorithms accept.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace optim = pluss::subsp::math::optim;
  using Pack = optim::ParameterPack<pluss::Series>;
  const auto f64 = pluss::ScalarType::Float64;
  auto scalar = [&](double v) { return pluss::make_series(v).to(f64); };
  const int n = 6;
  auto name = [](int i) { return "x" + std::to_string(i); };
  using Block = optim::ParameterPack<pluss::Series>::Block;
  auto filled = [&](double v) {
    std::vector<Block> blocks;
    for (int i = 0; i < n; ++i) blocks.push_back({name(i), scalar(v).reshape({1})});
    return optim::ParameterPack<pluss::Series>(blocks);
  };
  auto start = [&] {
    std::vector<Block> blocks;
    for (int i = 0; i < n; ++i) blocks.push_back({name(i), scalar(i % 2 == 0 ? -1.2 : 1.0).reshape({1})});
    return optim::ParameterPack<pluss::Series>(blocks);
  }();

  auto objective = [&](const Pack& p) {
    pluss::Series total = p.at(name(0)) * 0.0;
    for (int i = 0; i + 1 < n; ++i) {
      total = total + 100.0 * (p.at(name(i + 1)) - p.at(name(i)).square()).square() +
              (1.0 - p.at(name(i))).square();
    }
    return total;
  };
  const auto box = optim::constraints().lbs(filled(-3.0)).ubs(filled(3.0));

  struct Model { const char* label; optim::OptimaHessianApproximation model; };
  const std::vector<Model> models = {{"Bfgs", optim::OptimaHessianApproximation::Bfgs},
                                     {"Lbfgs", optim::OptimaHessianApproximation::Lbfgs},
                                     {"Identity", optim::OptimaHessianApproximation::Identity}};

  std::cout << std::left << std::setw(18) << "algorithm" << std::setw(10) << "model" << std::setw(6) << "flag"
            << std::setw(7) << "iters" << std::setw(7) << "evals" << "objective\n";
  auto run = [&](const char* label, optim::OptimaOptions options, const Model& m) {
    const auto r = optim::optima(objective, start, options, box);
    std::cout << std::left << std::setw(18) << label << std::setw(10) << m.label << std::setw(6)
              << static_cast<int>(r.exit_flag) << std::setw(7) << r.iteration << std::setw(7)
              << r.function_evaluations << std::scientific << std::setprecision(2)
              << r.evaluation.value.item<double>() << std::defaultfloat << "\n";
  };
  for (const auto& m : models) {
    optim::OptimaOptions options;
    options.algorithm = optim::OptimaAlgorithm::SequentialQP;
    options.sequential_qp.hessian_approximation = m.model;
    options.sequential_qp.max_iterations = 300;
    run("SequentialQP", options, m);
  }
  for (const auto& m : models) {
    optim::OptimaOptions options;
    options.algorithm = optim::OptimaAlgorithm::InternalProtocol;
    options.internal_protocol.hessian_approximation = m.model;
    options.internal_protocol.max_iterations = 300;
    run("InternalProtocol", options, m);
  }
  return 0;
}
```

Output

```text
algorithm         model     flag  iters  evals  objective
SequentialQP      Bfgs      1     56     61     1.44e-10
SequentialQP      Lbfgs     1     56     64     1.13e-09
SequentialQP      Identity  0     300    301    5.10e+00
InternalProtocol  Bfgs      1     57     65     1.51e-08
InternalProtocol  Lbfgs     1     51     64     5.71e-09
InternalProtocol  Identity  0     300    302    5.07e+00
```
The identity model uses no curvature information at all, and it shows what the other models are worth: neither algorithm finishes in 300 iterations with it, and both stop with objective values around `5` against below `1e-7` for the real models. Between the real models the choice is smaller. `Bfgs` stores a dense matrix, which is fine here and large for thousands of variables; `Lbfgs` stores a few recent steps and rebuilds the matrix from them, and it needed fewer iterations than `Bfgs` in the interior method (51 against 57) and the same number in sequential QP (56). All three algorithms accept all of these models. Two more, `Exact` and `HessianVector`, use exact second derivatives, which the library gets by differentiating your own functions a second time; chapter 6 compares all four, and chapter 7 shows how to supply the functions.

## Program 6: Badly scaled problems

A problem whose variables or constraints are written in very different units is the classic stress test for step-based methods. This one is 1e8 times steeper in one variable than in the other, has variables four thousand times apart in size, and states its equality in units of 1e-6. The table solves it with every algorithm under each `problem_scaling` mode.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::subsp::math::optim;
  using Pack = optim::ParameterPack<pluss::Series>;
  const auto f64 = pluss::ScalarType::Float64;
  auto scalar = [&](double v) { return pluss::make_series(v).to(f64); };
  auto two = [&](double a, double b) { return optim::pack_scalars({{"a", scalar(a)}, {"b", scalar(b)}}); };

  // minimize 1e8 (a - 0.3)^2 + 1e-4 (b - 1250)^2  subject to  1e-6 (a + b - 1250.3) = 0  and  a, b >= 0
  auto objective = [](const Pack& p) {
    return 1.0e8 * (p.at("a") - 0.3).square() + 1.0e-4 * (p.at("b") - 1250.0).square();
  };
  const auto constraints =
      optim::constraints()
          .nl_eq([](const Pack& p) { return 1.0e-6 * (p.at("a") + p.at("b") - 1250.3); })
          .lbs(two(0.0, 0.0));

  struct Mode { const char* label; optim::OptimaProblemScaling scaling; };
  const std::vector<Mode> modes = {{"None", optim::OptimaProblemScaling::None},
                                   {"InitialPoint", optim::OptimaProblemScaling::InitialPoint},
                                   {"ObjectiveAndConstraints", optim::OptimaProblemScaling::ObjectiveAndConstraints}};
  struct Row { const char* label; optim::OptimaAlgorithm algorithm; };
  const std::vector<Row> rows = {{"MeritMethod", optim::OptimaAlgorithm::MeritMethod},
                                 {"SequentialQP", optim::OptimaAlgorithm::SequentialQP},
                                 {"WorkingSet", optim::OptimaAlgorithm::WorkingSet},
                                 {"InternalProtocol", optim::OptimaAlgorithm::InternalProtocol}};

  std::cout << std::left << std::setw(18) << "iterations (flag)";
  for (const auto& m : modes) std::cout << std::setw(26) << m.label;
  std::cout << "\n";
  for (const auto& row : rows) {
    std::cout << std::left << std::setw(18) << row.label;
    for (const auto& mode : modes) {
      optim::OptimaOptions options;
      options.algorithm = row.algorithm;
      options.sequential_qp.problem_scaling = mode.scaling;
      options.working_set.problem_scaling = mode.scaling;
      options.internal_protocol.problem_scaling = mode.scaling;
      const auto r = optim::optima(objective, two(1.0, 1000.0), options, constraints);
      std::cout << std::setw(26) << (std::to_string(r.iteration) + " (" + std::to_string(static_cast<int>(r.exit_flag)) + ")");
    }
    std::cout << "\n";
  }
  return 0;
}
```

Output

```text
iterations (flag) None                      InitialPoint              ObjectiveAndConstraints   
MeritMethod       9 (1)                     9 (1)                     9 (1)                     
SequentialQP      2 (1)                     2 (1)                     3 (1)                     
WorkingSet        2 (1)                     2 (1)                     3 (1)                     
InternalProtocol  5 (1)                     5 (1)                     6 (1)                     
```
Every one of the twelve runs converges, in at most nine iterations. The scaling mode changes the iteration count by at most one, so on this problem it is not a decision you need to make. (The merit method has no scaling option, so its row is the same in every column.) The default mode, `ObjectiveAndConstraints`, solves a rescaled copy of your problem, with the objective and each constraint multiplied by a constant that brings its gradient at the start to a reasonable size, and reports the answer in your own units; that, together with judging optimality relative to the size of the starting gradient, is why the units of your constraints do not decide whether a run succeeds. Chapter 6 measures it on problems written in far more extreme units than this one.

## Program 8: Growing the problem

The cost of a method shows in how its work grows with the number of variables. The problem is a smoothing fit with `n` variables, `2(n - 1)` difference constraints, one equality and bounds, whose solution has many active constraints. Iterations and function evaluations are shown; an asterisk would mark a run that did not converge.

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace optim = pluss::subsp::math::optim;
  const auto f64 = pluss::ScalarType::Float64;

  struct Row { const char* label; optim::OptimaAlgorithm algorithm; };
  const std::vector<Row> rows = {{"MeritMethod", optim::OptimaAlgorithm::MeritMethod},
                                 {"SequentialQP", optim::OptimaAlgorithm::SequentialQP},
                                 {"WorkingSet", optim::OptimaAlgorithm::WorkingSet},
                                 {"InternalProtocol", optim::OptimaAlgorithm::InternalProtocol}};

  std::cout << std::left << std::setw(18) << "iters / evals";
  for (int n : {8, 16, 32}) std::cout << std::setw(14) << ("n = " + std::to_string(n));
  std::cout << "\n";

  for (const auto& row : rows) {
    std::cout << std::left << std::setw(18) << row.label;
    for (int n : {8, 16, 32}) {
      std::vector<double> target(n), start(n), lo(n, 0.0), hi(n, 1.0);
      double total = 0.0;
      for (int i = 0; i < n; ++i) {
        target[i] = 0.2 + 0.6 * i / (n - 1) + 0.15 * std::sin(1.7 * i);
        start[i] = (i % 2 == 0) ? 0.9 : 0.1;
        total += target[i];
      }
      std::vector<double> diff(static_cast<std::size_t>(2 * (n - 1) * n), 0.0);
      std::vector<double> cap(static_cast<std::size_t>(2 * (n - 1)), 0.12);
      for (int i = 0; i + 1 < n; ++i) {
        diff[static_cast<std::size_t>((2 * i) * n + i)] = 1.0;
        diff[static_cast<std::size_t>((2 * i) * n + i + 1)] = -1.0;
        diff[static_cast<std::size_t>((2 * i + 1) * n + i)] = -1.0;
        diff[static_cast<std::size_t>((2 * i + 1) * n + i + 1)] = 1.0;
      }
      auto vec = [&](const std::vector<double>& v) {
        return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
      };
      const auto goal = vec(target);
      auto objective = [&](const pluss::Series& x) { return (x - goal).square().sum(); };
      const auto constraints =
          optim::constraints()
              .ln_ineq(pluss::make_series(diff, {2 * (n - 1), n}).to(f64), vec(cap))
              .ln_eq(pluss::make_series(std::vector<double>(n, 1.0), {1, n}).to(f64), vec({total}))
              .lbs(vec(lo))
              .ubs(vec(hi));
      optim::OptimaOptions options;
      options.algorithm = row.algorithm;
      options.penalty.max_iterations = 2000;
      options.sequential_qp.max_iterations = 500;
      options.working_set.max_iterations = 500;
      options.internal_protocol.max_iterations = 500;
      const auto r = optim::optima(objective, vec(start), options, constraints);
      std::cout << std::setw(14)
                << (std::to_string(r.iteration) + " / " + std::to_string(r.function_evaluations) +
                    (r.exit_flag == optim::OptimaExitFlag::Converged ? "" : " *"));
    }
    std::cout << "\n";
  }
  std::cout << "* did not converge\n";
  return 0;
}
```

Output

```text
iters / evals     n = 8         n = 16        n = 32        
MeritMethod       38 / 67       60 / 119      84 / 196      
SequentialQP      7 / 8         8 / 9         8 / 13        
WorkingSet        7 / 8         12 / 28       11 / 20       
InternalProtocol  12 / 16       15 / 19       14 / 18       
* did not converge
```
Sequential QP, the working-set method and the interior method each need 7 to 15 iterations at every size, and between 8 and 28 evaluations. The merit method needs more iterations as the problem grows (38, 60, 84) and more evaluations (67, 119, 196). When each evaluation is expensive, evaluations are the number to watch: sequential QP is the cheapest at every size (8, 9 and 13), the interior method comes next (16, 19 and 18), and the working-set method is the most expensive at 16 variables (28).

## Decisions and pitfalls

- **Never run with no curvature model on a coupled problem.** The identity model uses no curvature information and shows what the others are worth: it fails to finish in 300 iterations where the real models need a handful.
- **Use a limited-memory model for thousands of variables.** `Bfgs` stores a dense matrix; `Lbfgs` stores a few recent steps and scales with the problem.
- **Scaling usually does not need a decision.** The default mode rescales a copy of your problem; on this problem the iteration count changed by at most one. Change it only when you can show a difference.
- **Count evaluations, not iterations, when the objective is expensive.** The cheapest method in iterations can be the dearest in evaluations, and the other way round.
- **Test at the size you will use.** A method that is cheap at ten variables can grow badly; the merit method's iteration count rises with the problem where the quadratic-model methods stay flat.

Where next: [Options and presets](options_and_presets.md) shows which settings move the result and how to avoid setting the wrong ones.
