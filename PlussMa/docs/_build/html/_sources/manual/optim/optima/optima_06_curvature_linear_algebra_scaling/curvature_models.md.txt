<!-- generated from curvature_models.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Curvature models

[Back to Optima 6: Curvature, Linear Algebra and Scaling](../optima_06_curvature_linear_algebra_scaling.md)

The three quadratic-model algorithms need second-derivative information, and `hessian_approximation` chooses where it comes from: the identity (none at all), a dense BFGS approximation, a limited-memory BFGS approximation, or the exact Hessian, obtained by differentiating your functions a second time. This page measures what each choice costs and buys.

The first program grows the problem and compares the models by iterations and evaluations, which shows that the identity model never converges and the real models grow gently. The second varies `lbfgs_memory`, the number of recent step pairs the limited-memory model remembers. The third runs every algorithm with every model on one problem and shows the one remaining refusal (an exact model on an objective given only as a value and a gradient is rejected, never silently replaced). The fourth looks at the BFGS safeguard, the curvature condition that decides whether an update is accepted.

## Program 1: How the model scales with the problem

The Rosenbrock chain, with its strongly coupled curvature, is solved for 6, 12 and 24 variables. Each cell is iterations and function evaluations.

```cpp
#include <pluss/ma.h>

#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;
using Block = optim::ParameterPack<pluss::Series>::Block;
const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

// The Rosenbrock chain: n variables, strongly coupled curvature, boxed to [-3, 3], started at (-1.2, 1, -1.2, ...).
struct Chain {
  int n;
  std::string name(int i) const { return "x" + std::to_string(i); }
  optim::ParameterPack<pluss::Series> filled(double lo_or_hi) const {
    std::vector<Block> b;
    for (int i = 0; i < n; ++i) b.push_back({name(i), scalar(lo_or_hi).reshape({1})});
    return optim::ParameterPack<pluss::Series>(b);
  }
  optim::ParameterPack<pluss::Series> start() const {
    std::vector<Block> b;
    for (int i = 0; i < n; ++i) b.push_back({name(i), scalar(i % 2 == 0 ? -1.2 : 1.0).reshape({1})});
    return optim::ParameterPack<pluss::Series>(b);
  }
  pluss::Series objective(const Pack& p) const {
    pluss::Series total = p.at(name(0)) * 0.0;
    for (int i = 0; i + 1 < n; ++i)
      total = total + 100.0 * (p.at(name(i + 1)) - p.at(name(i)).square()).square() + (1.0 - p.at(name(i))).square();
    return total;
  }
  auto box() const { return optim::constraints().lbs(filled(-3.0)).ubs(filled(3.0)); }
  optim::OptimaState solve(const optim::OptimaOptions& o) const {
    return optim::optima([this](const Pack& p) { return objective(p); }, start(), o, box());
  }
};

int main() {
  struct Col { const char* label; optim::OptimaAlgorithm alg; optim::OptimaHessianApproximation model; };
  const Col cols[] = {
      {"SequentialQP Bfgs", optim::OptimaAlgorithm::SequentialQP, optim::OptimaHessianApproximation::Bfgs},
      {"Interior Bfgs", optim::OptimaAlgorithm::InternalProtocol, optim::OptimaHessianApproximation::Bfgs},
      {"Interior Lbfgs", optim::OptimaAlgorithm::InternalProtocol, optim::OptimaHessianApproximation::Lbfgs},
      {"Interior Identity", optim::OptimaAlgorithm::InternalProtocol, optim::OptimaHessianApproximation::Identity}};
  std::cout << std::left << std::setw(8) << "n";
  for (const auto& c : cols) std::cout << std::setw(22) << c.label;
  std::cout << "\n";
  for (int n : {6, 12, 24}) {
    std::cout << std::left << std::setw(8) << n;
    for (const auto& c : cols) {
      optim::OptimaOptions o;
      o.algorithm = c.alg;
      o.sequential_qp.hessian_approximation = c.model;
      o.internal_protocol.hessian_approximation = c.model;
      o.sequential_qp.max_iterations = 300;
      o.internal_protocol.max_iterations = 300;
      const auto r = Chain{n}.solve(o);
      std::cout << std::setw(22)
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
n       SequentialQP Bfgs     Interior Bfgs         Interior Lbfgs        Interior Identity     
6       56 / 61               57 / 65               51 / 64               300 / 302 *           
12      85 / 97               88 / 102              80 / 91               300 / 302 *           
24      145 / 162             145 / 155             137 / 161             300 / 302 *           
* did not converge
```
The real models grow gently: sequential QP with BFGS needs 56, 85 and 145 iterations; the interior method with BFGS 57, 88 and 145; with the limited-memory model 51, 80 and 137. The limited-memory model is the cheapest in iterations here, not just in storage. The identity model, which throws curvature away, does not converge in 300 iterations at any size. Curvature information is the difference between a method that finishes and one that crawls; the choice between BFGS and its limited-memory version is a smaller matter that depends on size and storage.

## Program 2: The memory of the limited-memory model

`lbfgs_memory` is the number of recent step pairs the model remembers.

```cpp
#include <pluss/ma.h>

#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;
using Block = optim::ParameterPack<pluss::Series>::Block;
const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

// The Rosenbrock chain: n variables, strongly coupled curvature, boxed to [-3, 3], started at (-1.2, 1, -1.2, ...).
struct Chain {
  int n;
  std::string name(int i) const { return "x" + std::to_string(i); }
  optim::ParameterPack<pluss::Series> filled(double lo_or_hi) const {
    std::vector<Block> b;
    for (int i = 0; i < n; ++i) b.push_back({name(i), scalar(lo_or_hi).reshape({1})});
    return optim::ParameterPack<pluss::Series>(b);
  }
  optim::ParameterPack<pluss::Series> start() const {
    std::vector<Block> b;
    for (int i = 0; i < n; ++i) b.push_back({name(i), scalar(i % 2 == 0 ? -1.2 : 1.0).reshape({1})});
    return optim::ParameterPack<pluss::Series>(b);
  }
  pluss::Series objective(const Pack& p) const {
    pluss::Series total = p.at(name(0)) * 0.0;
    for (int i = 0; i + 1 < n; ++i)
      total = total + 100.0 * (p.at(name(i + 1)) - p.at(name(i)).square()).square() + (1.0 - p.at(name(i))).square();
    return total;
  }
  auto box() const { return optim::constraints().lbs(filled(-3.0)).ubs(filled(3.0)); }
  optim::OptimaState solve(const optim::OptimaOptions& o) const {
    return optim::optima([this](const Pack& p) { return objective(p); }, start(), o, box());
  }
};

int main() {
  std::cout << std::left << std::setw(10) << "memory" << std::setw(6) << "flag" << std::setw(7) << "iters" << std::setw(7)
            << "evals" << "objective\n";
  for (std::int64_t memory : {1, 3, 5, 10, 30}) {
    optim::OptimaOptions o;
    o.algorithm = optim::OptimaAlgorithm::InternalProtocol;
    o.internal_protocol.hessian_approximation = optim::OptimaHessianApproximation::Lbfgs;
    o.internal_protocol.lbfgs_memory = memory;
    o.internal_protocol.max_iterations = 300;
    const auto r = Chain{12}.solve(o);
    std::cout << std::left << std::setw(10) << memory << std::setw(6) << static_cast<int>(r.exit_flag) << std::setw(7)
              << r.iteration << std::setw(7) << r.function_evaluations << std::scientific << std::setprecision(2)
              << r.evaluation.value.template item<double>() << "\n";
  }
  return 0;
}
```

Output

```text
memory    flag  iters  evals  objective
1         1     142    178    1.59e-05
3         1     86     101    5.16e-08
5         1     83     98     2.35e-07
10        1     80     91     1.69e-07
30        1     74     90     7.43e-09
```
Memory 1 needs 142 iterations and ends at an objective of 1.59e-05; memory 30 needs 74 and reaches 7.43e-09. The default of 10 (80 iterations, 1.69e-07) is already close to the best. A very short memory is a poor model, and a long one costs storage and arithmetic for little gain.

## Program 3: Every algorithm, every model

All three algorithms accept all four models. The exact models need no extra callbacks: they differentiate the problem's own functions a second time, so each function is written once, on plain `Series`. The problem is the four-variable benchmark with one nonlinear inequality and one nonlinear equality.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::subsp::math::optim;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }
optim::ParameterPack<pluss::Series> four(double a, double b, double c, double d) {
  return optim::pack_scalars({{"a", scalar(a)}, {"b", scalar(b)}, {"c", scalar(c)}, {"d", scalar(d)}});
}

pluss::Series objective_of(const optim::ParameterPack<pluss::Series>& p) {
  return p.at("a") * p.at("d") * (p.at("a") + p.at("b") + p.at("c")) + p.at("c");
}
pluss::Series inequality_of(const optim::ParameterPack<pluss::Series>& p) {
  return 25.0 - p.at("a") * p.at("b") * p.at("c") * p.at("d");
}
pluss::Series equality_of(const optim::ParameterPack<pluss::Series>& p) {
  return p.at("a").square() + p.at("b").square() + p.at("c").square() + p.at("d").square() - 40.0;
}

optim::ConstrainedProblem problem(bool objective_as_series) {
  optim::ConstrainedProblem prob;
  if (objective_as_series) {
    prob.objective.autodiff = objective_of;
  } else {
    prob.objective.analytic = [](const optim::ParameterPack<pluss::Series>& p) {
      const double a = as_f64(p.at("a")), b = as_f64(p.at("b")), c = as_f64(p.at("c")), d = as_f64(p.at("d"));
      return std::pair{scalar(a * d * (a + b + c) + c), four(d * (2 * a + b + c), a * d, a * d + 1.0, a * (a + b + c))};
    };
  }
  prob.nonlinear.inequality = inequality_of;
  prob.nonlinear.equality = equality_of;
  prob.bounds.lower = four(1, 1, 1, 1);
  prob.bounds.upper = four(5, 5, 5, 5);
  return prob;
}

int main() {
  struct Alg { const char* label; optim::OptimaAlgorithm alg; };
  struct Model { const char* label; optim::OptimaHessianApproximation model; };
  const Alg algs[] = {{"SequentialQP", optim::OptimaAlgorithm::SequentialQP},
                      {"WorkingSet", optim::OptimaAlgorithm::WorkingSet},
                      {"InternalProtocol", optim::OptimaAlgorithm::InternalProtocol}};
  const Model models[] = {{"Bfgs", optim::OptimaHessianApproximation::Bfgs},
                          {"Lbfgs", optim::OptimaHessianApproximation::Lbfgs},
                          {"Exact", optim::OptimaHessianApproximation::Exact},
                          {"HessianVector", optim::OptimaHessianApproximation::HessianVector}};

  std::cout << "cell = iterations / evaluations    objective\n";
  std::cout << std::left << std::setw(18) << "algorithm";
  for (const auto& m : models) std::cout << std::setw(16) << m.label;
  std::cout << "\n";
  double worst_gap = 0.0;
  for (const auto& a : algs) {
    std::cout << std::left << std::setw(18) << a.label;
    for (const auto& m : models) {
      optim::OptimaOptions o;
      o.algorithm = a.alg;
      o.sequential_qp.hessian_approximation = m.model;
      o.working_set.hessian_approximation = m.model;
      o.internal_protocol.hessian_approximation = m.model;
      const auto r = optim::optima(problem(true), four(1, 5, 5, 1), o);
      worst_gap = std::max(worst_gap, std::abs(r.evaluation.value.template item<double>() - 17.0140172891));
      std::cout << std::setw(16) << (std::to_string(r.iteration) + " / " + std::to_string(r.function_evaluations));
    }
    std::cout << "\n";
  }
  std::cout << "largest distance of any cell from the optimum: " << std::scientific << std::setprecision(1) << worst_gap << "\n\n";

  optim::OptimaOptions exact;
  exact.algorithm = optim::OptimaAlgorithm::SequentialQP;
  exact.sequential_qp.hessian_approximation = optim::OptimaHessianApproximation::Exact;
  const auto refused = optim::optima(problem(false), four(1, 5, 5, 1), exact);
  std::cout << "Exact on a value-and-gradient objective: exit flag " << static_cast<int>(refused.exit_flag) << "\n"
            << refused.message.substr(0, refused.message.find(" diagnostic")) << "\n";
  return 0;
}
```

Output

```text
cell = iterations / evaluations    objective
algorithm         Bfgs            Lbfgs           Exact           HessianVector   
SequentialQP      4 / 5           6 / 8           5 / 6           5 / 6           
WorkingSet        4 / 6           6 / 9           5 / 7           5 / 7           
InternalProtocol  8 / 12          9 / 13          14 / 33         14 / 33         
largest distance of any cell from the optimum: 6.2e-07

Exact on a value-and-gradient objective: exit flag -5
exact curvature requires an autodiff objective and autodiff nonlinear callbacks (hessian_approximation is Exact or HessianVector)
```
All twelve cells reach the optimum of the problem, 17.0140173, to within 6.2e-07 of it. The cheapest in evaluations is the dense BFGS model, which on this small problem needs 5, 6 and 12 evaluations in the three algorithms; the exact models are close in the first two (6 and 7) and more expensive in the interior method (33). On this small problem the cheap quasi-Newton models are as good as the exact ones, and the table does not show an advantage for the exact models. They give the true curvature at once instead of learning it from the steps, and they cost a Hessian-vector product per variable per iteration (each one runs your functions again), so they pay off where the quasi-Newton model learns slowly or where the Hessian is cheap to evaluate.

The last two lines show the one remaining refusal. An exact model on an objective given only as a value and a gradient (`objective.analytic`) cannot be differentiated again; it is rejected with exit flag `-5` and a message that names what is missing, rather than quietly using another model, so a run never reports results for a model it did not use. The exact Hessian is made positive definite where it has to be: the shift is chosen so that the curvature is positive on the directions the active constraints leave free, not on the whole space, so a full Newton step is kept whenever that reduced curvature is already positive.

## Program 4: The BFGS safeguard

A BFGS update is only valid when the step and the change in gradient point the same way (the curvature condition). A pair that fails it is skipped, and `bfgs_curvature_tolerance` sets how strict that test is.

```cpp
#include <pluss/ma.h>

#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;
using Block = optim::ParameterPack<pluss::Series>::Block;
const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

// The Rosenbrock chain: n variables, strongly coupled curvature, boxed to [-3, 3], started at (-1.2, 1, -1.2, ...).
struct Chain {
  int n;
  std::string name(int i) const { return "x" + std::to_string(i); }
  optim::ParameterPack<pluss::Series> filled(double lo_or_hi) const {
    std::vector<Block> b;
    for (int i = 0; i < n; ++i) b.push_back({name(i), scalar(lo_or_hi).reshape({1})});
    return optim::ParameterPack<pluss::Series>(b);
  }
  optim::ParameterPack<pluss::Series> start() const {
    std::vector<Block> b;
    for (int i = 0; i < n; ++i) b.push_back({name(i), scalar(i % 2 == 0 ? -1.2 : 1.0).reshape({1})});
    return optim::ParameterPack<pluss::Series>(b);
  }
  pluss::Series objective(const Pack& p) const {
    pluss::Series total = p.at(name(0)) * 0.0;
    for (int i = 0; i + 1 < n; ++i)
      total = total + 100.0 * (p.at(name(i + 1)) - p.at(name(i)).square()).square() + (1.0 - p.at(name(i))).square();
    return total;
  }
  auto box() const { return optim::constraints().lbs(filled(-3.0)).ubs(filled(3.0)); }
  optim::OptimaState solve(const optim::OptimaOptions& o) const {
    return optim::optima([this](const Pack& p) { return objective(p); }, start(), o, box());
  }
};

int main() {
  std::cout << std::left << std::setw(12) << "tolerance" << std::setw(18) << "algorithm" << std::setw(6) << "flag" << std::setw(7)
            << "iters" << std::setw(7) << "evals" << "objective\n";
  for (double tol : {1e-12, 1e-8, 1e-4, 1e-1, 1.0}) {
    for (const auto alg : {optim::OptimaAlgorithm::SequentialQP, optim::OptimaAlgorithm::InternalProtocol}) {
      optim::OptimaOptions o;
      o.algorithm = alg;
      o.sequential_qp.bfgs_curvature_tolerance = tol;
      o.internal_protocol.bfgs_curvature_tolerance = tol;
      o.sequential_qp.max_iterations = 300;
      o.internal_protocol.max_iterations = 300;
      const auto r = Chain{12}.solve(o);
      std::ostringstream t;
      t << tol;
      std::cout << std::left << std::setw(12) << t.str() << std::setw(18)
                << (alg == optim::OptimaAlgorithm::SequentialQP ? "SequentialQP" : "InternalProtocol") << std::setw(6)
                << static_cast<int>(r.exit_flag) << std::setw(7) << r.iteration << std::setw(7) << r.function_evaluations
                << std::scientific << std::setprecision(2) << r.evaluation.value.template item<double>() << "\n";
    }
  }
  return 0;
}
```

Output

```text
tolerance   algorithm         flag  iters  evals  objective
1e-12       SequentialQP      1     85     97     5.46e-10
1e-12       InternalProtocol  1     88     102    5.22e-09
1e-08       SequentialQP      1     85     97     5.46e-10
1e-08       InternalProtocol  1     88     102    5.22e-09
0.0001      SequentialQP      1     85     97     5.46e-10
0.0001      InternalProtocol  1     88     102    5.22e-09
0.1         SequentialQP      1     89     100    6.26e-09
0.1         InternalProtocol  1     93     107    4.32e-08
1           SequentialQP      0     300    301    1.10e+01
1           InternalProtocol  0     300    302    1.10e+01
```
From 1e-12 to 1e-4 the result is identical: the default is far below anything that matters. At 0.1 the cost shows (89 and 93 iterations against 85 and 88), and at 1 neither algorithm converges in 300 iterations, with an objective of 11.0 left. Do not raise this tolerance to "make the model more cautious": it makes the model stop learning.

## Decisions and pitfalls

- **The limited-memory model is cheap in iterations too, not only in storage.** On a coupled chain it needed the fewest iterations of the real models.
- **The default memory (10) is close to the best.** Memory 1 is a poor model; memory 30 buys little for its storage and arithmetic.
- **An exact Hessian needs an autodiff objective.** It differentiates your `Series` functions a second time; an objective given only as a value and a gradient is refused with exit flag `-5` rather than quietly using another model.
- **Do not raise `bfgs_curvature_tolerance` to be cautious.** Up to about 1e-4 the result is identical; at 1 the model stops learning and neither algorithm converges.
- **On a small problem the cheap models win.** Exact curvature costs more per iteration; it pays when the problem is difficult, not merely when it is available.

Where next: [The step's linear algebra, and units](linear_algebra_and_units.md) covers the system solved at every step and the effect of units.
