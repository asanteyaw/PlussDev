<!-- generated from the_poll.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The poll

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
  c.search_strategy = dfo::PSSearchStrategy::Off;  // isolate the poll
  const auto o = dfo::pattern_search_minimizer(rosenbrock, vec({-1.2, 1.0}), c);
  std::cout << std::left << std::setw(36) << label << std::setw(26) << dfo::to_string(o.status) << std::scientific << std::setprecision(2)
            << std::setw(11) << o.minimum << std::defaultfloat << "evals " << o.diagnostics.evaluations << "\n";
}

int main() {
  dfo::PSConfig base;
  run("2N, consecutive (default)", base);
  auto np1 = base; np1.poll_pattern = dfo::PSPollPattern::PositiveBasisNp1;
  run("N+1 basis", np1);
  auto rnd = base; rnd.polling_order = dfo::PSPollingOrder::Random;
  run("random order", rnd);
  auto sf = base; sf.polling_order = dfo::PSPollingOrder::SuccessFirst;
  run("success-first order", sf);
  auto full = base; full.complete_poll = true;
  run("complete poll", full);
  auto canon = base; canon.basis_family = dfo::PSBasisFamily::Canonical;
  run("coordinate-axis basis", canon);
}
```

Output

```text
2N, consecutive (default)           ReachedIterationBudget    1.51e-02   evals 664
N+1 basis                           ReachedIterationBudget    4.17e-02   evals 513
random order                        ReachedIterationBudget    2.07e-02   evals 641
success-first order                 ReachedIterationBudget    1.26e-02   evals 629
complete poll                       ReachedIterationBudget    9.21e-03   evals 801
coordinate-axis basis               ReachedIterationBudget    3.37e+00   evals 449
```
All rows used the same 200-iteration budget, so compare the minimum reached and the evaluations spent together.

- The **N+1 basis** polls n+1 points instead of 2n: fewer evaluations per iteration, a coarser look around.
- **Order** matters only when the poll stops at the first improvement. `SuccessFirst` tries the direction that last worked first and is usually best in a curved valley.
- **Complete poll** evaluates every direction and takes the best, costing more per iteration for a better step.
- The **coordinate-axis basis** cannot follow the diagonal Rosenbrock valley: it is the worst row by orders of magnitude. The default rotating basis exists for this reason. The coordinate basis is the right choice when the axes themselves are meaningful, for instance when each variable has its own units and coordinate steps make sense.
