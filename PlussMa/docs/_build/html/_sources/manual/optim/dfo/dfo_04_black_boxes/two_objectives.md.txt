<!-- generated from two_objectives.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Two objectives

[Back to Black Boxes](../dfo_04_black_boxes.md)


The black box returns a vector of objectives. The solver minimises one scalar built from them and also keeps the Pareto set, the points that no other evaluated point beats on every objective. The weights state how much each objective matters.

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  std::cout << std::left << std::setw(12) << "weight w" << std::setw(12) << "x" << std::setw(12) << "expected" << std::setw(12)
            << std::setw(12) << "f1" << std::setw(12) << "f2" << "pareto points\n";
  for (double w : {0.2, 0.5, 0.8}) {
    dfo::DfoProblem p;
    p.initial = vec({0.0});
    p.blackbox = [](const pluss::Series& x) {
      dfo::BlackboxEvaluation e;
      const auto f1 = (x - 1.0).square().sum().reshape({1});
      const auto f2 = (x + 1.0).square().sum().reshape({1});
      e.objectives = f1.unite({f2}, 0);
      return e;
    };
    dfo::DfoOptions o;
    o.mads.multiobjective_scalarization = dfo::MultiObjectiveScalarization::WeightedSum;
    o.mads.objective_weights = vec({w, 1.0 - w});
    const auto r = dfo::DFO::solve(p, o);
    const auto f = r.objective_vector.to_vec<double>();
    std::cout << std::fixed << std::setprecision(3) << std::left << std::setw(12) << w << std::setw(12)
              << r.minimizer.select(0, 0).item<double>() << std::setw(12) << 2.0 * w - 1.0 << std::setw(12) << std::setw(12) << f[0] << std::setw(12) << f[1] << r.pareto_points.size(0) << "\n";
  }
}
```

Output

```text
weight w    x           expected    f1          f2          pareto points
0.200       -0.600      -0.600      2.560       0.160       71
0.500       0.000       0.000       1.000       1.000       61
0.800       0.600       0.600       0.160       2.560       71
```
The weights slide the answer along the trade-off between the two objectives, exactly as the formula says. `result.pareto_points` and `result.pareto_objectives` hold the non-dominated points (as rows) and their objective vectors. They are an archive of the search, not a computed front: they cover the region the search visited, which is densest near the chosen compromise. To map a whole front, run several weightings and gather their archives.
