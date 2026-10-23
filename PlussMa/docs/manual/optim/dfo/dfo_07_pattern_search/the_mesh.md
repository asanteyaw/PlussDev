<!-- generated from the_mesh.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The mesh

[Back to The Pattern Search Minimiser](../dfo_07_pattern_search.md)


```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series rosenbrock(const pluss::Series& x) {
  const auto a = x.select(0, 0), b = x.select(0, 1);
  return (b - a * a).square() * 100.0 + (a * -1.0 + 1.0).square();
}

void run(const char* label, dfo::PSConfig c) {
  c.lower_bounds = vec({-10.0, -5000.0});
  c.upper_bounds = vec({10.0, 5000.0});
  auto f = [](const pluss::Series& x) { return (x.select(0, 0) - 3.0).square() + ((x.select(0, 1) - 2000.0) / 1000.0).square(); };
  const auto o = dfo::pattern_search_minimizer(f, vec({0.0, 0.0}), c);
  std::cout << std::left << std::setw(34) << label << std::setw(26) << dfo::to_string(o.status) << std::scientific << std::setprecision(2)
            << std::setw(11) << o.minimum << std::defaultfloat << "evals " << o.diagnostics.evaluations << "\n";
}

int main() {
  dfo::PSConfig base;
  run("defaults (RangeLog scaling)", base);
  auto off = base; off.mesh_scaling = dfo::PSMeshScaling::Off;
  run("mesh_scaling Off", off);
  auto nups = base; nups.mesh_scaling = dfo::PSMeshScaling::NupsDynamic;
  run("mesh_scaling NupsDynamic", nups);
  auto small = base; small.initial_mesh_size = 0.01;
  run("initial_mesh_size 0.01", small);
  auto large = base; large.initial_mesh_size = 100.0;
  run("initial_mesh_size 100", large);
  auto slow = base; slow.mesh_contraction = 0.9; slow.mesh_expansion = 1.1;
  run("expansion 1.1, contraction 0.9", slow);
}
```

Output

```text
defaults (RangeLog scaling)       Converged                 1.41e-10   evals 221
mesh_scaling Off                  ReachedIterationBudget    6.76e-03   evals 660
mesh_scaling NupsDynamic          Converged                 2.57e-03   evals 505
initial_mesh_size 0.01            Converged                 9.34e-11   evals 201
initial_mesh_size 100             Converged                 3.19e-11   evals 277
expansion 1.1, contraction 0.9    ReachedIterationBudget    9.98e-11   evals 786
```
The problem has one variable of order 1 and one of order 1000. With bounds supplied, `RangeLog` scales each variable's step by its range, so the poll behaves as if both were of order one, and the run converges accurately. Without scaling, a step suited to one variable is far too small or too large for the other. The initial mesh size matters much less than the scaling: the mesh grows and shrinks by factors of 2 and 0.5, so a poor start is corrected in a few iterations. Gentler factors (1.1 and 0.9) work, but need many more iterations.

Scaling is derived from the bounds. Without bounds there is no range to read, so give the variables comparable scales yourself (divide by a typical size inside the objective) or supply bounds.
