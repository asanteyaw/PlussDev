<!-- generated from restarting_a_local_method.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Restarting a local method

[Back to Choosing a Solver](../dfo_02_the_solvers.md)


When the problem has many local minima and the evaluation budget allows, restart a cheap local method from several starts and keep the best.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  const double pi = 3.14159265358979323846;
  auto rastrigin = [pi](const pluss::Series& x) { return (x.square() - (x * (2.0 * pi)).cos() * 10.0 + 10.0).sum(); };
  const std::vector<std::vector<double>> starts = {{3.2, 2.7}, {-2.1, 3.4}, {1.4, -1.6}, {-3.6, -2.4}, {0.6, 0.7}, {2.2, -2.9}};

  double best = 1e300;
  std::int64_t total = 0;
  std::cout << std::left << std::setw(16) << "start" << std::setw(14) << "minimum" << std::setw(10) << "evals"
            << std::setw(14) << "best so far" << "total evals\n";
  for (const auto& s : starts) {
    dfo::DfoProblem p;
    p.initial = vec(s);
    p.objective = rastrigin;
    dfo::DfoOptions o;
    o.solver = dfo::DfoSolverKind::NelderMead;
    const auto r = dfo::DFO::solve(p, o);
    total += r.diagnostics.evaluations;
    best = std::min(best, r.minimum);
    std::cout << std::left << "(" << std::fixed << std::setprecision(1) << std::setw(5) << s[0] << "," << std::setw(5) << s[1] << ") "
              << std::scientific << std::setprecision(2) << std::setw(14) << r.minimum << std::defaultfloat << std::setw(10)
              << r.diagnostics.evaluations << std::scientific << std::setw(14) << best << std::defaultfloat << total << "\n";
  }
}
```

Output

```text
start           minimum       evals     best so far   total evals
(3.2  ,2.7  ) 1.79e+01      60        1.79e+01      60
(-2.1 ,3.4  ) 1.99e+01      105       1.79e+01      165
(1.4  ,-1.6 ) 4.97e+00      56        4.97e+00      221
(-3.6 ,-2.4 ) 1.99e+01      61        4.97e+00      282
(0.6  ,0.7  ) 1.99e+00      67        1.99e+00      349
(2.2  ,-2.9 ) 1.29e+01      56        1.99e+00      405
```
Six restarts cost 405 evaluations in total and improved the best value from 17.9 to 1.99. None reached the global minimum 0, because none started in its basin. That is the honest limit of restarts: they help in proportion to how well the starts cover the region. A population method covers the region by construction, which is why it found the global minimum above, at far more evaluations. Pick the strategy by the evaluations you can afford.
