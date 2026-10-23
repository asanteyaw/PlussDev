<!-- generated from the_mesh_and_the_frame.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The mesh and the frame

[Back to Mesh Adaptive Direct Search](../dfo_10_mesh_adaptive_search.md)


```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series filled(std::int64_t n, double v) { return pluss::make_series(std::vector<double>(n, v), {n}).to(f64); }
pluss::Series rosenbrock(const pluss::Series& x) {
  const auto a = x.select(0, 0), b = x.select(0, 1);
  return (b - a * a).square() * 100.0 + (a * -1.0 + 1.0).square();
}
const char* name(dfo::BlackboxSolverStatus s) {
  return s == dfo::BlackboxSolverStatus::Converged ? "Converged"
         : s == dfo::BlackboxSolverStatus::ReachedEvaluationBudget ? "ReachedEvaluationBudget" : "ReachedIterationBudget";
}
void run(const char* label, dfo::MadsBlackboxConfig c) {
  auto f = [](const pluss::Series& x) {
    const auto d = x - vec({1.0, -1.0, 2.0, 0.5});
    const auto s = d.select(0, 0) + d.select(0, 1) + d.select(0, 2) + d.select(0, 3);
    return d.square().sum() + s.square() * 0.8 + 1.0;
  };
  c.max_iterations = 4000;
  c.max_evaluations = 40000;
  const auto o = dfo::orthomads_minimizer(f, vec({3.0, 2.0, -1.0, 1.0}), c);
  std::cout << std::left << std::setw(32) << label << std::setw(26) << name(o.status) << std::scientific << std::setprecision(2) << std::setw(11)
            << o.minimum << std::defaultfloat << "evals " << o.diagnostics.evaluations << "\n";
}

int main() {
  dfo::MadsBlackboxConfig base;
  run("defaults", base);
  auto big = base; big.initial_frame_size = 8.0; big.initial_mesh_size = 8.0;
  run("initial frame and mesh 8", big);
  auto tiny = base; tiny.initial_frame_size = 0.01; tiny.initial_mesh_size = 0.01;
  run("initial frame and mesh 0.01", tiny);
  auto gentle = base; gentle.frame_refine_factor = 0.8; gentle.mesh_refine_factor = 0.64;
  run("gentle refinement (0.8, 0.64)", gentle);
  auto aggressive = base; aggressive.frame_refine_factor = 0.25; aggressive.mesh_refine_factor = 0.0625;
  run("aggressive refinement", aggressive);
  auto tight = base; tight.check_every = 0; tight.mesh_tolerance = 1e-12;
  run("mesh_tolerance 1e-12, no window", tight);
  auto mid = base; mid.check_every = 0; mid.mesh_tolerance = 1e-8;
  run("mesh_tolerance 1e-8, no window", mid);
  auto loose = base; loose.check_every = 0; loose.mesh_tolerance = 1e-3;
  run("mesh_tolerance 1e-3, no window", loose);
  auto full = base; full.opportunistic = false;
  run("opportunistic off", full);
}
```

Output

```text
defaults                        Converged                 1.00e+00   evals 880
initial frame and mesh 8        Converged                 1.00e+00   evals 839
initial frame and mesh 0.01     Converged                 1.00e+00   evals 834
gentle refinement (0.8, 0.64)   Converged                 1.00e+00   evals 2905
aggressive refinement           Converged                 1.00e+00   evals 659
mesh_tolerance 1e-12, no window Converged                 1.00e+00   evals 984
mesh_tolerance 1e-8, no window  Converged                 1.00e+00   evals 880
mesh_tolerance 1e-3, no window  Converged                 1.00e+00   evals 409
opportunistic off               Converged                 1.00e+00   evals 1281
```
- The **starting scale** hardly matters: the frame grows or shrinks by factors of 2 and 0.5 until it fits.
- **Refinement speed** matters. Shrinking gently (0.8) polls many times at nearly the same scale and costs several times the evaluations; shrinking quickly (0.25) is cheaper.
- **`mesh_tolerance`** is the accuracy you ask for. With the window test off, it alone ends the run, and tightening it from 1e-3 to 1e-12 costs about two and a half times the evaluations.
- **Opportunistic off** evaluates the whole poll every time and takes the best point: more evaluations per iteration.
