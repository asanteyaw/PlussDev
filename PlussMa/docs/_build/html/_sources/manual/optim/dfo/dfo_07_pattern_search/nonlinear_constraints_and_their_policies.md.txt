<!-- generated from nonlinear_constraints_and_their_policies.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Nonlinear constraints and their policies

[Back to The Pattern Search Minimiser](../dfo_07_pattern_search.md)


`nonlinear_constraints` returns a `PSConstraintValues` with an `inequality` vector (feasible when every entry is `<= 0`) and an `equality` vector (`= 0`).

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

int main() {
  struct P { const char* name; dfo::PSConstraintPolicy p; };
  const P rows[] = {{"ExtremeBarrier", dfo::PSConstraintPolicy::ExtremeBarrier},
                    {"MeritPenalty", dfo::PSConstraintPolicy::MeritPenalty},
                    {"AugmentedLagrangian", dfo::PSConstraintPolicy::AugmentedLagrangian}};
  std::cout << std::left << std::setw(22) << "policy" << std::setw(26) << "status" << std::setw(11) << "objective" << std::setw(12) << "violation"
            << std::setw(8) << "evals" << "outer rounds\n";
  for (const auto& r : rows) {
    dfo::PSConfig c;
    c.constraint_policy = r.p;
    c.nonlinear_constraints = [](const pluss::Series& x) {
      dfo::PSConstraintValues v;
      v.inequality = (x.square().sum() - 1.0).reshape({1});
      return v;
    };
    auto f = [](const pluss::Series& x) { return (x - 2.0).square().sum(); };
    c.max_iterations = 4000;
    c.max_evaluations = 40000;
    const auto o = dfo::pattern_search_minimizer(f, vec({0.0, 0.0}), c);
    std::cout << std::left << std::setw(22) << r.name << std::setw(26) << dfo::to_string(o.status) << std::fixed << std::setprecision(4)
              << std::setw(11) << o.minimum << std::scientific << std::setprecision(1) << std::setw(12) << o.diagnostics.max_constraint_violation
              << std::defaultfloat << std::setw(8) << o.diagnostics.evaluations << o.diagnostics.outer_iterations << "\n";
  }
}
```

Output

```text
policy                status                    objective  violation   evals   outer rounds
ExtremeBarrier        Converged                 3.3437     0.0e+00     121     0
MeritPenalty          Converged                 3.3433     0.0e+00     239     0
AugmentedLagrangian   Converged                 3.3431     7.6e-07     1398    8
```
- **ExtremeBarrier** rejects any violating point. Cheapest, and always feasible, but it can only approach a boundary from inside, so it stops a little short.
- **MeritPenalty** adds a penalty on the violation to the objective and increases it as needed. It reaches closer to the boundary.
- **AugmentedLagrangian** (the default) adds multiplier estimates to the penalty and refines them over several outer rounds. It costs the most evaluations and gives the most accurate answer, and it is the policy built for equality constraints, which a barrier cannot satisfy (a random point never lies exactly on one). Its violation is small but not zero.
