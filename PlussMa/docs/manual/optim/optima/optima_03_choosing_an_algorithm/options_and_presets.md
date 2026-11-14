<!-- generated from options_and_presets.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Options and presets

[Back to Optima 3: Choosing and Tuning an Algorithm](../optima_03_choosing_an_algorithm.md)

Each algorithm has its own options struct with dozens of settings, and most of them do little on a well-behaved problem. This page tells you where to look first. It sweeps the few settings that moved the result on the earlier problem, shows the three **presets** that return a complete `OptimaOptions` with the algorithm chosen and the tolerances set for a typical constrained fit, and ends with the most common configuration mistake: setting an option on the *wrong* struct.

That mistake deserves its place because it fails silently. Each algorithm reads only its own struct, so a setting made on another algorithm's struct is ignored without a message. The default algorithm is SQP, which makes the slip easy in both directions: you ask the interior method for something while the run is using SQP, or the reverse. The last program shows the symptom (a run that ignored your iteration limit) and the fix.

## Program 7: The options that decide the outcome

Most of the options in each algorithm's struct do little on a well-behaved problem. This program sweeps the ones that moved the result on the problem from Program 1, so you know where to look first.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

int main() {
  namespace optim = pluss::subsp::math::optim;
  using Pack = optim::ParameterPack<pluss::Series>;
  const auto f64 = pluss::ScalarType::Float64;
  auto scalar = [&](double v) { return pluss::make_series(v).to(f64); };
  auto four = [&](double a, double b, double c, double d) {
    return optim::pack_scalars({{"a", scalar(a)}, {"b", scalar(b)}, {"c", scalar(c)}, {"d", scalar(d)}});
  };
  auto objective = [](const Pack& p) {
    return p.at("a") * p.at("d") * (p.at("a") + p.at("b") + p.at("c")) + p.at("c");
  };
  const auto constraints =
      optim::constraints()
          .nl_ineq([](const Pack& p) { return 25.0 - p.at("a") * p.at("b") * p.at("c") * p.at("d"); })
          .nl_eq([](const Pack& p) {
            return p.at("a").square() + p.at("b").square() + p.at("c").square() +
                   p.at("d").square() - 40.0;
          })
          .lbs(four(1, 1, 1, 1))
          .ubs(four(5, 5, 5, 5));

  std::cout << std::left << std::setw(52) << "setting" << std::setw(6) << "flag" << std::setw(7) << "iters"
            << std::setw(7) << "evals" << "objective\n";
  auto fmt = [](double v) {
    std::ostringstream out;
    out << v;
    return out.str();
  };
  auto run = [&](const std::string& label, const optim::OptimaOptions& options) {
    const auto r = optim::optima(objective, four(1, 5, 5, 1), options, constraints);
    std::cout << std::left << std::setw(52) << label << std::setw(6) << static_cast<int>(r.exit_flag)
              << std::setw(7) << r.iteration << std::setw(7) << r.function_evaluations << std::fixed
              << std::setprecision(4) << r.evaluation.value.item<double>() << std::defaultfloat << "\n";
  };

  for (double mu : {1e-4, 1e-2, 1.0}) {
    optim::OptimaOptions o;
    o.algorithm = optim::OptimaAlgorithm::InternalProtocol;
    o.internal_protocol.initial_barrier_parameter = mu;
    run("InternalProtocol initial_barrier_parameter " + fmt(mu), o);
  }
  for (double f : {0.9, 0.99}) {
    optim::OptimaOptions o;
    o.algorithm = optim::OptimaAlgorithm::InternalProtocol;
    o.internal_protocol.fraction_to_boundary = f;
    run("InternalProtocol fraction_to_boundary " + fmt(f), o);
  }
  for (bool on : {true, false}) {
    optim::OptimaOptions o;
    o.algorithm = optim::OptimaAlgorithm::InternalProtocol;
    o.internal_protocol.independent_nonlinear_slacks = on;
    run(std::string("InternalProtocol independent_nonlinear_slacks ") + (on ? "true" : "false"), o);
  }
  for (double tol : {1e-3, 1e-6, 1e-10}) {
    optim::OptimaOptions o;
    o.algorithm = optim::OptimaAlgorithm::SequentialQP;
    o.sequential_qp.optimality_tolerance = tol;
    run("SequentialQP optimality_tolerance " + fmt(tol), o);
  }
  for (double penalty : {1.0, 10.0, 100.0}) {
    optim::OptimaOptions o;
    o.algorithm = optim::OptimaAlgorithm::MeritMethod;
    o.penalty.penalty = penalty;
    run("MeritMethod penalty " + fmt(penalty), o);
  }
  return 0;
}
```

Output

```text
setting                                             flag  iters  evals  objective
InternalProtocol initial_barrier_parameter 0.0001   1     15     26     17.0140
InternalProtocol initial_barrier_parameter 0.01     1     8      12     17.0140
InternalProtocol initial_barrier_parameter 1        1     18     43     17.0140
InternalProtocol fraction_to_boundary 0.9           1     10     14     17.0140
InternalProtocol fraction_to_boundary 0.99          1     8      12     17.0140
InternalProtocol independent_nonlinear_slacks true  1     8      12     17.0140
InternalProtocol independent_nonlinear_slacks false 1     56     293    17.0140
SequentialQP optimality_tolerance 0.001             1     4      5      17.0140
SequentialQP optimality_tolerance 1e-06             1     4      5      17.0140
SequentialQP optimality_tolerance 1e-10             1     5      6      17.0140
MeritMethod penalty 1                               1     33     84     17.0140
MeritMethod penalty 10                              1     68     180    27.1464
MeritMethod penalty 100                             1     79     283    17.0140
```
Four lessons. The initial barrier parameter of the interior method is worth a quick sweep: too small and too large both cost iterations, and the default `0.01` was the best of the three. Backing off `fraction_to_boundary` from `0.99` to `0.9` costs two iterations here. `independent_nonlinear_slacks` is the single largest effect in the table, 56 iterations and 293 evaluations without it against 8 and 12 with it, because without independent slacks a start on the boundary of a nonlinear inequality can only creep along the curve. Tightening sequential QP's `optimality_tolerance` from `1e-6` to `1e-10` costs one more iteration, from 4 to 5, with no change visible at four decimals. And the merit method's `penalty` does not just change the path: with `10` it settles at the other first-order point, `27.1464`, while `1` and `100` reach `17.0140`. When one algorithm's answer looks wrong, run it again with a different penalty before blaming the model.

## Program 9: The ready-made option sets

Three presets return a complete `OptimaOptions` with the algorithm chosen and the tolerances set for a typical constrained fit. The program prints what they change from the defaults and runs each on the problem from Program 1.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace optim = pluss::subsp::math::optim;

int main() {
  using Pack = optim::ParameterPack<pluss::Series>;
  const auto f64 = pluss::ScalarType::Float64;
  auto scalar = [&](double v) { return pluss::make_series(v).to(f64); };
  auto four = [&](double a, double b, double c, double d) {
    return optim::pack_scalars({{"a", scalar(a)}, {"b", scalar(b)}, {"c", scalar(c)}, {"d", scalar(d)}});
  };
  auto objective = [](const Pack& p) {
    return p.at("a") * p.at("d") * (p.at("a") + p.at("b") + p.at("c")) + p.at("c");
  };
  const auto constraints =
      optim::constraints()
          .nl_ineq([](const Pack& p) { return 25.0 - p.at("a") * p.at("b") * p.at("c") * p.at("d"); })
          .nl_eq([](const Pack& p) {
            return p.at("a").square() + p.at("b").square() + p.at("c").square() +
                   p.at("d").square() - 40.0;
          })
          .lbs(four(1, 1, 1, 1))
          .ubs(four(5, 5, 5, 5));

  const optim::OptimaOptions defaults;
  struct Preset { const char* label; optim::OptimaOptions options; };
  const std::vector<Preset> presets = {{"defaults", defaults},
                                       {"sequential_qp()", optim::presets::sequential_qp()},
                                       {"internal_protocol()", optim::presets::internal_protocol()},
                                       {"robust_internal_protocol()", optim::presets::robust_internal_protocol()}};

  std::cout << std::left << std::setw(28) << "options" << std::setw(18) << "algorithm" << std::setw(12)
            << "max iters" << std::setw(16) << "constraint tol" << std::setw(13) << "optimal tol" << "scaling\n";
  for (const auto& p : presets) {
    const auto& o = p.options;
    const bool interior = o.algorithm == optim::OptimaAlgorithm::InternalProtocol;
    const auto iters = interior ? o.internal_protocol.max_iterations : o.sequential_qp.max_iterations;
    const auto ctol = interior ? o.internal_protocol.constraint_tolerance : o.sequential_qp.constraint_tolerance;
    const auto otol = interior ? o.internal_protocol.optimality_tolerance : o.sequential_qp.optimality_tolerance;
    const auto scaling = interior ? o.internal_protocol.problem_scaling : o.sequential_qp.problem_scaling;
    std::cout << std::left << std::setw(28) << p.label << std::setw(18) << optim::optima_algorithm_name(o.algorithm)
              << std::setw(12) << iters << std::setw(16) << ctol << std::setw(13) << otol
              << optim::optima_problem_scaling_name(scaling) << "\n";
  }

  std::cout << "\nrunning each on the same problem\n";
  for (const auto& p : presets) {
    const auto r = optim::optima(objective, four(1, 5, 5, 1), p.options, constraints);
    std::cout << "  " << std::left << std::setw(28) << p.label << "flag " << std::setw(3)
              << static_cast<int>(r.exit_flag) << "iterations " << std::setw(4) << r.iteration << "violation "
              << std::scientific << std::setprecision(1) << r.evaluation.max_constraint_violation
              << std::defaultfloat << "\n";
  }
  return 0;
}
```

Output

```text
options                     algorithm         max iters   constraint tol  optimal tol  scaling
defaults                    SequentialQP      100         1e-06           1e-06        ObjectiveAndConstraints
sequential_qp()             SequentialQP      100         1e-08           1e-06        InitialPoint
internal_protocol()         InternalProtocol  100         1e-08           1e-06        InitialPoint
robust_internal_protocol()  InternalProtocol  160         1e-08           1e-06        InitialPoint

running each on the same problem
  defaults                    flag 1  iterations 4   violation 8.2e-08
  sequential_qp()             flag 1  iterations 5   violation 3.6e-11
  internal_protocol()         flag 1  iterations 9   violation 5.3e-12
  robust_internal_protocol()  flag 1  iterations 9   violation 5.3e-12
```
The presets differ from the defaults in tighter constraint tolerance (`1e-8` against `1e-6`) and in turning on scaling from the starting point. `robust_internal_protocol()` raises the iteration limit to 160 and widens the safeguards, so it matters on problems that need more room, not on this one. If you start from a preset and then change one setting, you keep the rest of its choices.

## Program 10: Setting an option on the wrong struct

Each algorithm reads only its own struct, so a setting made on another algorithm's struct is silently ignored. The default algorithm is sequential QP, which makes the mistake easy in both directions. The program then shows the setting done right: sequential QP reduces its quadratic subproblem to a symmetric positive-definite system, and the linear-solver and Krylov options on its own struct choose how that system is solved (Cholesky by default, conjugate gradients on request or for large problems).

```cpp
#include <pluss/ma.h>

#include <iostream>

int main() {
  namespace optim = pluss::subsp::math::optim;
  using Pack = optim::ParameterPack<pluss::Series>;
  const auto f64 = pluss::ScalarType::Float64;
  auto start = optim::pack_scalars({{"a", pluss::make_series(0.0).to(f64)}, {"b", pluss::make_series(0.0).to(f64)}});
  auto objective = [](const Pack& p) {
    return (p.at("a") - 3.0).square() + (p.at("b") - 3.0).square();
  };
  const auto disc = optim::constraints().nl_ineq(
      [](const Pack& p) { return p.at("a").square() + p.at("b").square() - 2.0; });

  optim::OptimaOptions wrong;
  wrong.algorithm = optim::OptimaAlgorithm::InternalProtocol;
  wrong.sequential_qp.max_iterations = 2;  // not the struct of the algorithm that runs
  const auto ignored = optim::optima(objective, start, wrong, disc);

  optim::OptimaOptions right;
  right.algorithm = optim::OptimaAlgorithm::InternalProtocol;
  right.internal_protocol.max_iterations = 2;
  const auto honored = optim::optima(objective, start, right, disc);

  std::cout << "set on sequential_qp, running InternalProtocol: " << ignored.iteration << " iterations, flag "
            << static_cast<int>(ignored.exit_flag) << "\n";
  std::cout << "set on internal_protocol, running InternalProtocol: " << honored.iteration << " iterations, flag "
            << static_cast<int>(honored.exit_flag) << "\n";

  // The same problem with the subproblem solved by conjugate gradients.
  optim::OptimaOptions plain;
  plain.algorithm = optim::OptimaAlgorithm::SequentialQP;
  optim::OptimaOptions krylov = plain;
  krylov.sequential_qp.linear_system_solver = optim::OptimaLinearSystemSolver::ConjugateGradient;
  krylov.sequential_qp.max_krylov_iterations = 50;
  krylov.sequential_qp.system_structure = optim::OptimaLinearSystemStructure::Sparse;
  const auto a = optim::optima(objective, start, plain, disc);
  const auto b = optim::optima(objective, start, krylov, disc);
  std::cout << "sequential QP default:       " << a.iteration << " iterations, krylov_iterations "
            << a.output.krylov_iterations << "\n";
  std::cout << "sequential QP with Krylov:   " << b.iteration << " iterations, krylov_iterations "
            << b.output.krylov_iterations << "\n";

  // The default algorithm is not the interior method.
  const optim::OptimaOptions defaults;
  std::cout << "default algorithm is SequentialQP: " << std::boolalpha
            << (defaults.algorithm == optim::OptimaAlgorithm::SequentialQP) << "\n";
  return 0;
}
```

Output

```text
set on sequential_qp, running InternalProtocol: 6 iterations, flag 1
set on internal_protocol, running InternalProtocol: 2 iterations, flag 0
sequential QP default:       5 iterations, krylov_iterations 0
sequential QP with Krylov:   5 iterations, krylov_iterations 88
default algorithm is SequentialQP: true
```
The first run asked for at most 2 iterations on the wrong struct and ran 6 to convergence; the second asked on the right struct and stopped at 2 with flag `0`. Asking for conjugate gradients on the sequential QP struct changed how the subproblems were solved but not the path: both runs take 5 iterations, and `krylov_iterations` is `0` for the default and `88` when conjugate gradients were requested. When a setting seems to have no effect, check that it sits on the struct of the algorithm that is running.

## Decisions and pitfalls

- **Set an option on the struct of the algorithm that runs.** A setting on another algorithm's struct is silently ignored; if a setting seems to do nothing, check which struct you set it on.
- **Sweep a few options, not all.** The initial barrier parameter, `fraction_to_boundary` and `independent_nonlinear_slacks` of the interior method moved the result most; start there.
- **Presets are starting points.** If you start from a preset and change one setting, you keep the rest of its choices, including the tighter tolerances.
- **A change that does not change the path may still change the cost.** Asking for conjugate gradients changed how subproblems were solved (88 Krylov iterations against 0) but not the path; look at the work counters, not only the answer.

Where next: [Chapter 4](../optima_04_sequential_qp.md) opens up the default algorithm, sequential QP.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="curvature_scaling_and_size.html">
    <span class="label">Previous</span>
    <span class="title">Curvature, scaling and size</span>
  </a>
</div>
<!-- apiref:nav:end -->
