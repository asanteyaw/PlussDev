<!-- generated from bounds_only.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Bounds only

[Back to Bounds and Constraints](../dfo_03_bounds_and_constraints.md)


Every method keeps its trial points inside the box; the objective in this program counts the calls made outside it.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  double lowest = 1e300, highest = -1e300;
  std::int64_t outside = 0;
  auto f = [&](const pluss::Series& x) {
    const double a = x.select(0, 0).item<double>(), b = x.select(0, 1).item<double>();
    if (a < 0 || a > 2 || b < 0 || b > 2) ++outside;
    lowest = std::min({lowest, a, b});
    highest = std::max({highest, a, b});
    return (x - vec({3.0, -1.0})).square().sum();
  };
  for (auto k : {dfo::DfoSolverKind::NelderMead, dfo::DfoSolverKind::PatternSearch, dfo::DfoSolverKind::DifferentialEvolution,
                 dfo::DfoSolverKind::ParticleSwarm, dfo::DfoSolverKind::OrthoMads}) {
    lowest = 1e300; highest = -1e300; outside = 0;
    dfo::DfoProblem p;
    p.initial = vec({1.0, 1.0});
    p.objective = f;
    p.lower_bounds = vec({0.0, 0.0});
    p.upper_bounds = vec({2.0, 2.0});
    dfo::DfoOptions o;
    o.solver = k;
    o.max_evaluations = 5000;
    const auto r = dfo::DFO::solve(p, o);
    std::cout << std::left << std::setw(24) << dfo::to_string(k) << std::fixed << std::setprecision(4) << "(" << r.minimizer.select(0, 0).item<double>()
              << ", " << r.minimizer.select(0, 1).item<double>() << ")  f " << r.minimum << "  evaluated outside box: " << outside << "\n";
  }
}
```

Output

```text
NelderMead              (2.0000, 0.0000)  f 2.0000  evaluated outside box: 0
PatternSearch           (2.0000, 0.0000)  f 2.0000  evaluated outside box: 0
DifferentialEvolution   (2.0000, 0.0000)  f 2.0000  evaluated outside box: 0
ParticleSwarm           (2.0000, 0.0000)  f 2.0000  evaluated outside box: 0
OrthoMads               (2.0000, 0.0000)  f 2.0000  evaluated outside box: 0
```
The optimum sits on a corner, where two bounds are active. All five methods find it. `NelderMead` does it by projecting: the simplex is free to move, but each point is clamped into the box before the objective sees it.
