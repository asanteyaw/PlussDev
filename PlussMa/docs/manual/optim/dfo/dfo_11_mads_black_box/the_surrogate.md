<!-- generated from the_surrogate.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The surrogate

[Back to The Black-Box MADS Minimisers](../dfo_11_mads_black_box.md)


```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series filled(std::int64_t n, double v) { return pluss::make_series(std::vector<double>(n, v), {n}).to(f64); }
const char* name(dfo::BlackboxSolverStatus s) {
  return s == dfo::BlackboxSolverStatus::Converged ? "Converged"
         : s == dfo::BlackboxSolverStatus::ReachedEvaluationBudget ? "ReachedEvaluationBudget" : "ReachedIterationBudget";
}
int main() {
  std::cout << std::left << std::setw(6) << "dim" << std::setw(20) << "surrogate_mode" << std::setw(14) << "status" << std::setw(8) << "evals" << "orderings\n";
  for (std::int64_t dim : {4, 8, 12}) {
    std::vector<double> w(dim);
    for (std::int64_t i = 0; i < dim; ++i) w[i] = static_cast<double>(i + 1);
    const auto weights = vec(w);
    for (auto mode : {dfo::SurrogateMode::Off, dfo::SurrogateMode::CandidateOrdering}) {
      dfo::MadsBlackboxAdvancedConfig c;
      c.surrogate_mode = mode;
      c.max_iterations = 5000;
      c.max_evaluations = 20000;
      auto blackbox = [&](const pluss::Series& x) {
        dfo::BlackboxEvaluation e;
        e.objectives = ((x - 1.0).square() * weights).sum().reshape({1});
        return e;
      };
      const auto o = dfo::mads_blackbox_minimizer(blackbox, filled(dim, -1.0), c);
      std::cout << std::left << std::setw(6) << dim << std::setw(20) << (mode == dfo::SurrogateMode::Off ? "Off" : "CandidateOrdering")
                << std::setw(14) << name(o.status) << std::setw(8) << o.diagnostics.evaluations << o.diagnostics.surrogate_orderings << "\n";
    }
  }
}
```

Output

```text
dim   surrogate_mode      status        evals   orderings
4     Off                 Converged     1454    0
4     CandidateOrdering   Converged     1250    212
8     Off                 Converged     5205    0
8     CandidateOrdering   Converged     4678    424
12    Off                 Converged     13228   0
12    CandidateOrdering   Converged     10398   644
```
The model never replaces an evaluation, so the answer is as reliable as without it; it only reorders each poll. On this smooth function it saves a tenth to a fifth of the evaluations, more in higher dimension, where polls are longer. The saving is bounded by construction: ordering can only shorten the part of a poll that comes after the first improving point, and an unsuccessful poll must still evaluate every candidate, so no model can remove those evaluations. On a rough function the ordering helps less. It is worth trying when each evaluation is expensive and the function is reasonably smooth.
