<!-- generated from seeds_and_repeatability.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Seeds and repeatability

[Back to First Problems](../dfo_01_first_problems.md)


Methods that use random numbers read them from `rng_seed`. Same seed, same everything; different seed, different path.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series filled(std::int64_t n, double v) { return pluss::make_series(std::vector<double>(n, v), {n}).to(f64); }

int main() {
  const double pi = 3.14159265358979323846;
  auto rastrigin = [pi](const pluss::Series& x) { return (x.square() - (x * (2.0 * pi)).cos() * 10.0 + 10.0).sum(); };
  std::cout << std::left << std::setw(8) << "seed" << std::setw(26) << "status" << std::setw(14) << "minimum" << "evaluations\n";
  for (std::uint64_t seed : {1ULL, 2ULL, 1ULL, 5489ULL, 5489ULL}) {
    dfo::DfoProblem p;
    p.initial = filled(3, 3.0);
    p.lower_bounds = filled(3, -5.12);
    p.upper_bounds = filled(3, 5.12);
    p.objective = rastrigin;
    dfo::DfoOptions o;
    o.solver = dfo::DfoSolverKind::DifferentialEvolution;
    o.max_evaluations = 4000;
    o.rng_seed = seed;
    const auto r = dfo::DFO::solve(p, o);
    std::cout << std::left << std::setw(8) << seed << std::setw(26) << dfo::to_string(r.status) << std::scientific
              << std::setprecision(3) << std::setw(14) << r.minimum << std::defaultfloat << r.diagnostics.evaluations << "\n";
  }
}
```

Output

```text
seed    status                    minimum       evaluations
1       ReachedEvaluationBudget   4.858e+00     4000
2       ReachedEvaluationBudget   4.955e+00     4000
1       ReachedEvaluationBudget   4.858e+00     4000
5489    ReachedEvaluationBudget   4.729e+00     4000
5489    ReachedEvaluationBudget   4.729e+00     4000
```
Rows 1 and 3 are identical, and so are rows 4 and 5. The Rastrigin function used here has many local minima, so different seeds end in different ones; with a fixed budget, running several seeds and keeping the best is a legitimate strategy for such problems.
