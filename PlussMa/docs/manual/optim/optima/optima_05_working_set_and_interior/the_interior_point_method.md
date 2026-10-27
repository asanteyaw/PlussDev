<!-- generated from the_interior_point_method.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The interior-point method

[Back to Optima 5: The Working-Set and Interior Methods](../optima_05_working_set_and_interior.md)

The interior-point method replaces each inequality by a **slack variable** and a **barrier term** that grows without bound as the slack approaches zero. It follows a path of barrier problems towards the solution, with a *barrier parameter* that is reduced as the path is followed. Bounds and linear rows are kept *strictly* satisfied at every iterate, which is the property that sets it apart: the iterates never touch the boundary.

The three programs show the method from its own side. The first prints the trace with the barrier parameter, the complementarity (the product of each slack and its multiplier, which must go to zero at a solution) and the stationarity. The second varies the starting barrier parameter and the other options of the schedule. The third shows the benefit: a function with a logarithm in it, defined only for positive arguments, is minimised from a start far from the minimum, and the interior method never evaluates it where it is undefined.

## Program 4: Watching the barrier

The interior method replaces each inequality by a slack variable and a barrier term. The trace shows the barrier parameter, the complementarity (the product of each slack and its multiplier, which must go to zero at a solution) and the stationarity.

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;
const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

// The same four-variable problem as in chapter 3: one nonlinear inequality, one nonlinear equality, bounds.
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }
optim::ParameterPack<pluss::Series> four(double a, double b, double c, double d) {
  return optim::pack_scalars({{"a", scalar(a)}, {"b", scalar(b)}, {"c", scalar(c)}, {"d", scalar(d)}});
}
auto hs71 = [](const Pack& p) { return p.at("a") * p.at("d") * (p.at("a") + p.at("b") + p.at("c")) + p.at("c"); };
auto hs71_constraints() {
  return optim::constraints()
      .nl_ineq([](const Pack& p) { return 25.0 - p.at("a") * p.at("b") * p.at("c") * p.at("d"); })
      .nl_eq([](const Pack& p) {
        return p.at("a").square() + p.at("b").square() + p.at("c").square() + p.at("d").square() - 40.0;
      })
      .lbs(four(1, 1, 1, 1))
      .ubs(four(5, 5, 5, 5));
}

int main() {
  optim::OptimaOptions options;
  options.algorithm = optim::OptimaAlgorithm::InternalProtocol;
  std::cout << std::right << std::setw(5) << "iter" << std::setw(12) << "barrier" << std::setw(16) << "complementarity"
            << std::setw(14) << "stationarity" << std::setw(12) << "violation" << std::setw(12) << "objective\n";
  options.progress_hook = [](const optim::OptimaProgressSnapshot& s) {
    if (s.stage != optim::OptimaProgressStage::Iteration) return false;
    std::cout << std::setw(5) << s.iteration << std::scientific << std::setprecision(2) << std::setw(12) << s.backend_parameter
              << std::setw(16) << s.complementarity << std::setw(14) << s.stationarity << std::setw(12)
              << s.evaluation.max_constraint_violation << std::fixed << std::setprecision(5) << std::setw(12)
              << s.evaluation.value.template item<double>() << "\n";
    return false;
  };
  const auto r = optim::optima(hs71, four(1, 5, 5, 1), options, hs71_constraints());
  std::cout << "\nexit flag " << static_cast<int>(r.exit_flag) << ", " << r.iteration << " iterations, "
            << r.function_evaluations << " evaluations\n";
  return 0;
}
```

Output

```text
 iter     barrier complementarity  stationarity   violation  objective
    1    1.00e-02        1.00e-02      1.00e+00    1.13e+01    16.05813
    2    1.00e-02        8.48e-03      1.07e+00    1.10e+00    16.51976
    3    1.00e-02        9.60e-03      2.17e-01    9.64e-01    16.38129
    4    1.00e-02        9.81e-03      1.64e-02    1.77e-01    16.89763
    5    1.00e-03        1.15e-02      2.51e-03    2.20e-03    17.03423
    6    3.16e-05        1.54e-03      6.97e-04    3.06e-04    17.01538
    7    1.78e-07        3.34e-05      7.47e-05    9.14e-06    17.01404
    8    1.00e-08        3.34e-07      3.44e-06    8.84e-08    17.01402

exit flag 1, 8 iterations, 12 evaluations
```
The barrier parameter stays at 1.00e-02 for four iterations, then falls to 1.00e-03, 3.16e-05, 1.78e-07 and finally its floor 1.00e-08. It falls only when the current barrier problem is solved well enough, which is why the first four iterations are spent at the starting value. Complementarity ends at 3.34e-07 and stationarity at 3.44e-06, both near the tolerances, and the violation falls from 11.3 to 8.84e-08. The same problem took four iterations with sequential QP; the interior method trades more iterations for iterates that stay strictly inside bounds and slacks.

## Program 5: The barrier schedule

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;
const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

// The same four-variable problem as in chapter 3: one nonlinear inequality, one nonlinear equality, bounds.
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }
optim::ParameterPack<pluss::Series> four(double a, double b, double c, double d) {
  return optim::pack_scalars({{"a", scalar(a)}, {"b", scalar(b)}, {"c", scalar(c)}, {"d", scalar(d)}});
}
auto hs71 = [](const Pack& p) { return p.at("a") * p.at("d") * (p.at("a") + p.at("b") + p.at("c")) + p.at("c"); };
auto hs71_constraints() {
  return optim::constraints()
      .nl_ineq([](const Pack& p) { return 25.0 - p.at("a") * p.at("b") * p.at("c") * p.at("d"); })
      .nl_eq([](const Pack& p) {
        return p.at("a").square() + p.at("b").square() + p.at("c").square() + p.at("d").square() - 40.0;
      })
      .lbs(four(1, 1, 1, 1))
      .ubs(four(5, 5, 5, 5));
}

int main() {
  std::cout << std::left << std::setw(44) << "setting" << std::setw(6) << "flag" << std::setw(7) << "iters" << std::setw(7)
            << "evals" << "objective\n";
  auto run = [&](const std::string& label, optim::OptimaOptions o) {
    o.algorithm = optim::OptimaAlgorithm::InternalProtocol;
    const auto r = optim::optima(hs71, four(1, 5, 5, 1), o, hs71_constraints());
    std::cout << std::left << std::setw(44) << label << std::setw(6) << static_cast<int>(r.exit_flag) << std::setw(7)
              << r.iteration << std::setw(7) << r.function_evaluations << std::fixed << std::setprecision(4)
              << r.evaluation.value.template item<double>() << "\n";
  };
  run("defaults", {});
  for (double mu : {1e-6, 1e-4, 1e-2, 1.0, 10.0}) {
    optim::OptimaOptions o;
    o.internal_protocol.initial_barrier_parameter = mu;
    std::ostringstream l; l << "initial_barrier_parameter " << mu;
    run(l.str(), o);
  }
  for (double d : {0.05, 0.2, 0.5, 0.9}) {
    optim::OptimaOptions o;
    o.internal_protocol.barrier_decay = d;
    std::ostringstream l; l << "barrier_decay " << d;
    run(l.str(), o);
  }
  for (double f : {0.9, 0.99, 0.999}) {
    optim::OptimaOptions o;
    o.internal_protocol.fraction_to_boundary = f;
    std::ostringstream l; l << "fraction_to_boundary " << f;
    run(l.str(), o);
  }
  return 0;
}
```

Output

```text
setting                                     flag  iters  evals  objective
defaults                                    1     8      12     17.0140
initial_barrier_parameter 1e-06             1     29     66     17.0140
initial_barrier_parameter 0.0001            1     15     26     17.0140
initial_barrier_parameter 0.01              1     8      12     17.0140
initial_barrier_parameter 1                 1     18     43     17.0140
initial_barrier_parameter 10                0     100    591    17.0439
barrier_decay 0.05                          1     8      12     17.0140
barrier_decay 0.2                           1     8      12     17.0140
barrier_decay 0.5                           1     8      12     17.0140
barrier_decay 0.9                           1     8      12     17.0140
fraction_to_boundary 0.9                    1     10     14     17.0140
fraction_to_boundary 0.99                   1     8      12     17.0140
fraction_to_boundary 0.999                  1     8      12     17.0140
```
The starting barrier parameter matters, and in both directions. The default 1e-2 needs 8 iterations. A much smaller start, 1e-6, needs 29 iterations and 66 evaluations: the method behaves like a nearly unpenalized Newton method and spends its steps being pushed back from the walls. A larger start, 1, needs 18 iterations, and 10 does not converge in 100 iterations (flag `0`, objective 17.0439). `barrier_decay` made no difference here, and `fraction_to_boundary` only mattered at 0.9, which costs 10 iterations against 8: a step that may use only 90% of the distance to the nearest wall is a shorter step. Leave all of these at their defaults unless a problem shows the symptoms in the table.

## Program 6: What "interior" buys

Bounds and linear rows are kept strictly satisfied at every iterate of the interior method. The program minimizes a function that contains a logarithm, so it is defined only for positive arguments, from a start far from the minimum, and records the smallest value any iterate takes.

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;
const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  auto f = [](const pluss::Series& x) { return x.sum() - 0.1 * x.log().sum(); };
  const auto cons = optim::constraints().lbs(vec({0.0, 0.0})).ubs(vec({10.0, 10.0}));
  const std::pair<const char*, optim::OptimaAlgorithm> algs[] = {
      {"InternalProtocol", optim::OptimaAlgorithm::InternalProtocol}, {"SequentialQP", optim::OptimaAlgorithm::SequentialQP}};
  std::cout << std::left << std::setw(18) << "algorithm" << std::setw(6) << "flag" << std::setw(7) << "iters"
            << std::setw(24) << "smallest value of x" << "objective\n";
  for (const auto& [name, alg] : algs) {
    optim::OptimaOptions o;
    o.algorithm = alg;
    double smallest = 1e300;
    o.progress_hook = [&](const optim::OptimaProgressSnapshot& s) {
      for (double v : s.params.at("x").host<double>()) smallest = std::min(smallest, v);
      return false;
    };
    const auto r = optim::optima(f, vec({9.0, 9.0}), o, cons);
    std::cout << std::left << std::setw(18) << name << std::setw(6) << static_cast<int>(r.exit_flag) << std::setw(7) << r.iteration
              << std::scientific << std::setprecision(2) << std::setw(24) << smallest << std::fixed << std::setprecision(6)
              << r.evaluation.value.template item<double>() << "\n";
  }
  return 0;
}
```

Output

```text
algorithm         flag  iters  smallest value of x     objective
InternalProtocol  1     11     3.06e-02                0.660517
SequentialQP      -6    3      5.40e-12                5.189051
```
The interior method converges in 11 iterations to 0.660517, and its smallest iterate is 3.06e-02: it never reaches the boundary. Sequential QP projects its steps onto the bounds and lands on `x = 0` to within rounding (5.40e-12), where the logarithm is huge. Its line search then fails (flag `-6`, objective 5.189051). If your objective is undefined outside the feasible region, this is the reason to use the interior method with `independent_nonlinear_slacks = false` for nonlinear inequalities as well (chapter 3, Program 2).

## Decisions and pitfalls

- **The barrier parameter falls only when the current barrier problem is solved well enough.** That is why a trace spends its first iterations at the starting value; that is normal.
- **Sweep the starting barrier parameter.** Too small makes the method behave like an unpenalised Newton method that spends its steps being pushed back from the walls; too large wastes iterations. The default (0.01) was the best of the values tried here.
- **Use the interior method when the objective is undefined outside the feasible region.** Its iterates never leave bounds and linear rows; SQP projects onto bounds and can land exactly where the objective is infinite.
- **Nonlinear inequalities are different.** By default each has its own slack, so the nonlinear constraints can be violated on the way; set `independent_nonlinear_slacks = false` to keep them strictly satisfied, and check the cost.
- **Complementarity is the quantity to watch.** It must fall to the tolerance together with the stationarity; a run that ends with large complementarity has stalled.

Where next: [Chapter 6](../optima_06_curvature_linear_algebra_scaling.md) looks at the curvature models and the linear algebra under every step.
