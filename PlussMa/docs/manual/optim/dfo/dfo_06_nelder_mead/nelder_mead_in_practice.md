<!-- generated from nelder_mead_in_practice.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Nelder–Mead in practice

[Back to Derivative-Free Optimization 6: The Nelder–Mead Minimiser](../dfo_06_nelder_mead.md)

Nelder–Mead is the simplest derivative-free method: it carries a set of n+1 points (a triangle in two variables) and repeatedly replaces the worst by reflecting it through the centre of the others, expanding or contracting as the result suggests. It needs no gradient and few evaluations on smooth problems of up to about ten variables, which is why `Auto` chooses it there. This page calls it directly with `nelder_mead_minimizer`, which exposes every setting.

The programs go from the plainest call to the details: a direct call, with the accuracy you can expect; each setting changed on its own (tolerances, starting simplex, coefficients, budget) so the effect of each is visible against the same baseline; forbidden regions, where the objective returns `+infinity` and the simplex moves away; and shapes and dtypes, where the parameter can be a matrix and the objective always receives the shape you gave.

## Program 1: A direct call

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  auto rosenbrock = [](const pluss::Series& x) {
    const auto a = x.select(0, 0), b = x.select(0, 1);
    return (b - a * a).square() * 100.0 + (a * -1.0 + 1.0).square();
  };
  const auto out = dfo::nelder_mead_minimizer(rosenbrock, vec({-1.2, 1.0}));
  std::cout << std::fixed << std::setprecision(5) << "minimizer  : " << out.minimizer.select(0, 0).item<double>() << ", "
            << out.minimizer.select(0, 1).item<double>() << "\n";
  std::cout << std::scientific << std::setprecision(2) << "minimum    : " << out.minimum << "\n";
  std::cout << "status     : " << (out.status == dfo::SolverStatus::Converged ? "Converged" : "budget") << "\n";
  std::cout << "iterations : " << out.diagnostics.iterations << "\n";
  std::cout << "evaluations: " << out.diagnostics.evaluations << "\n";
  std::cout << "last step  : " << out.diagnostics.last_step << "\n";
}
```

Output

```text
minimizer  : 1.00002, 1.00004
minimum    : 8.18e-10
status     : Converged
iterations : 85
evaluations: 159
last step  : adjust_inner
```
The accuracy is about 1e-5 in the variables: a function value of 1e-9 and a tolerance of 1e-4 on the simplex size. For more digits, tighten the tolerances (Program 2) or polish with another method (Chapter 5).

## Program 2: The settings, one at a time

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

#include <utility>
#include <vector>

namespace dfo = pluss::subsp::math::optimization;
using namespace pluss::tils;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  std::vector<std::pair<const char*, dfo::NelderMeadConfig>> cases;
  dfo::NelderMeadConfig base;
  cases.emplace_back("defaults", base);
  auto tight = base; tight.position_tolerance = 1e-8; tight.value_tolerance = 1e-8;
  cases.emplace_back("tolerances 1e-8", tight);
  auto loose = base; loose.position_tolerance = 1e-2; loose.value_tolerance = 1e-2;
  cases.emplace_back("tolerances 1e-2", loose);
  auto big = base; big.simplex_delta = 0.5;
  cases.emplace_back("simplex_delta 0.5", big);
  auto tiny = base; tiny.simplex_delta = 0.001;
  cases.emplace_back("simplex_delta 0.001", tiny);
  auto bold = base; bold.expansion = 3.0;
  cases.emplace_back("expansion 3", bold);
  auto timid = base; timid.contraction = 0.25; timid.shrink = 0.25;
  cases.emplace_back("contraction, shrink 0.25", timid);
  auto capped = base; capped.max_evaluations = 100;
  cases.emplace_back("max_evaluations 100", capped);

  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(2));
  for (auto [label, c] : cases) {
    auto f = [](const pluss::Series& x) {
      const auto d = x - vec({1.0, -1.0, 2.0, 0.5});
      const auto s = d.select(0, 0) + d.select(0, 1) + d.select(0, 2) + d.select(0, 3);
      return d.square().sum() + s.square() * 0.8;
    };
    const auto o = dfo::nelder_mead_minimizer(f, vec({3.0, 2.0, -1.0, 1.0}), c);
    out.writeln(label, dfo::to_string(o.status), o.minimum, "evals", o.diagnostics.evaluations);
  }
}
```

Output

```text
defaults Converged 1.72e-09 evals 240
tolerances 1e-8 Converged 1.58e-17 evals 385
tolerances 1e-2 Converged 1.80e-05 evals 177
simplex_delta 0.5 Converged 3.33e-09 evals 192
simplex_delta 0.001 Converged 1.47e-08 evals 400
expansion 3 Converged 2.20e-08 evals 318
contraction, shrink 0.25 Converged 3.16e-06 evals 478
max_evaluations 100 ReachedEvaluationBudget 2.09e+00 evals 100
```
What the rows show:

- **Tolerances** trade work for accuracy: 1e-8 costs about 60% more evaluations than the default and gains eight orders of magnitude in the objective; 1e-2 saves about a quarter.
- **Starting simplex.** A large simplex (0.5) explores more and here converged sooner; a tiny one (0.001) must grow before it can move, and took more than the default. Match the simplex to the scale of the problem: if the variables are of order 100, 5% is a step of 5.
- **Coefficients.** The defaults are the standard choice. Changing them rarely helps; the rows above are worse. Keep expansion above 1, contraction and shrink between 0 and 1.
- **Budget.** Stopping at 100 evaluations returns the best point seen with `ReachedEvaluationBudget`.

## Program 3: Forbidden regions

An objective can return `+infinity` for a point that is not allowed. The simplex sees a very bad value and moves away.

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  auto f = [](const pluss::Series& x) -> double {
    const double a = x.select(0, 0).item<double>(), b = x.select(0, 1).item<double>();
    if (a * a + b * b > 4.0) return std::numeric_limits<double>::infinity();
    return (a - 3) * (a - 3) + (b - 3) * (b - 3);
  };
  const auto o = dfo::nelder_mead_minimizer(f, vec({0.0, 0.0}));
  std::cout << std::fixed << std::setprecision(4) << "minimizer : " << o.minimizer.select(0, 0).item<double>() << ", "
            << o.minimizer.select(0, 1).item<double>() << "\n";
  std::cout << "minimum   : " << o.minimum << "  (exact 5.0294)\n";
  std::cout << "evaluations: " << o.diagnostics.evaluations << "\n";

  auto bad = [](const pluss::Series&) -> double { return std::nan(""); };
  try {
    (void)dfo::nelder_mead_minimizer(bad, vec({0.0, 0.0}));
  } catch (const std::exception&) {
    std::cout << "a NaN objective is rejected with an error\n";
  }
}
```

Output

```text
minimizer : 1.3744, 1.4529
minimum   : 5.0360  (exact 5.0294)
evaluations: 252
a NaN objective is rejected with an error
```
The result is the nearest point of the disc to about three digits. A simplex pressed against a wall stops a little short of the exact optimum, because most of its trial points fall in the forbidden region and are wasted. For a precise answer on a constraint boundary, use pattern search (Chapter 3). NaN and negative infinity are rejected with an error, since they cannot be ordered.

## Program 4: Shapes and dtypes

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

int main() {
  // Find the 2x2 matrix closest (in sum of squares) to a target.
  const auto target = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {2, 2}).to(pluss::ScalarType::Float64);
  auto f = [&](const pluss::Series& m) { return (m - target).square().sum(); };

  const auto start64 = pluss::make_series(std::vector<double>{0.0, 0.0, 0.0, 0.0}, {2, 2}).to(pluss::ScalarType::Float64);
  const auto a = dfo::nelder_mead_minimizer(f, start64);
  std::cout << "Float64 start -> shape " << a.minimizer.size(0) << "x" << a.minimizer.size(1) << ", dtype float64: "
            << (a.minimizer.scalar_type() == pluss::ScalarType::Float64) << ", minimum " << std::scientific << std::setprecision(1) << a.minimum << "\n";

  const auto start32 = start64.to(pluss::ScalarType::Float32);
  auto g = [&](const pluss::Series& m) { return (m.to(pluss::ScalarType::Float64) - target).square().sum(); };
  const auto b = dfo::nelder_mead_minimizer(g, start32);
  std::cout << "Float32 start -> shape " << b.minimizer.size(0) << "x" << b.minimizer.size(1) << ", dtype float32: "
            << (b.minimizer.scalar_type() == pluss::ScalarType::Float32) << ", minimum " << b.minimum << "\n";
}
```

Output

```text
Float64 start -> shape 2x2, dtype float64: 1, minimum 5.3e-09
Float32 start -> shape 2x2, dtype float32: 1, minimum 5.3e-09
```
The shape you give is the shape you get back, and the objective always receives that shape, so matrix-valued parameters need no flattening code of your own.

## Decisions and pitfalls

- **Match the starting simplex to the scale of the problem.** A step that is 5% of the variable is a step of 5 when the variable is of order 100; a tiny simplex must grow before it can move.
- **Keep the default coefficients.** Changing expansion, contraction or shrink rarely helps; keep expansion above 1 and the others between 0 and 1.
- **Tolerances trade work for accuracy.** Tightening to 1e-8 costs about 60% more evaluations here and gains eight orders of magnitude in the objective.
- **A simplex pressed against a wall stops short.** Most of its trial points fall in the forbidden region and are wasted; for a precise answer on a boundary use pattern search. NaN and negative infinity are rejected with an error.
- **Nelder–Mead can stall on non-smooth functions.** The simplex collapses against a kink while reporting `Converged`; check with an independent method.

Where next: [Chapter 7](../dfo_07_pattern_search.md) describes pattern search, the constrained method.
