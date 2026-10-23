<!-- generated from the_best_feasible_checkpoint.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The best feasible checkpoint

[Back to Reading Results and Watching a Run](../optima_02_reading_results.md)


A run that ends without converging may have visited a better point than its last. The solver keeps the best feasible point it saw. `result.best_feasible` holds it and `output.best_feasible_*` summarizes it. `returned_best_feasible` tells you whether the answer in `params` is the checkpoint or the final iterate.

```cpp
#include <pluss/ma.h>

#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::optim;
  using Pack = optim::ParameterPack<optim::Trace>;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  auto objective = [](const optim::Trace& x) { return optim::sum(optim::square(x - 3.0)); };
  const auto disc = optim::constraints().nl_ineq(
      [](const Pack& p) { return optim::sum(optim::square(p.at("x"))) - 1.0; });

  optim::OptimaOptions options;
  options.sequential_qp.max_iterations = 3;
  const auto r = optim::optima(objective, vec({0.0, 0.0}), options, disc);

  std::cout << std::boolalpha;
  std::cout << "exit flag " << static_cast<int>(r.exit_flag) << ", converged " << r.converged << "\n";
  std::cout << "returned answer: objective " << r.evaluation.value.item<double>() << ", violation "
            << r.evaluation.max_constraint_violation << "\n";
  std::cout << "best feasible available: " << r.output.best_feasible_available << "\n";
  std::cout << "answer is the checkpoint: " << r.output.returned_best_feasible << "\n";
  if (r.best_feasible) {
    const auto x = r.best_feasible->params.at("x").to_vec<double>();
    std::cout << "checkpoint point [" << x[0] << ", " << x[1] << "]\n";
    std::cout << "checkpoint objective " << r.best_feasible->value.item<double>() << ", violation "
              << r.best_feasible->max_constraint_violation << "\n";
    std::cout << "report matches checkpoint: "
              << (r.output.best_feasible_value == r.best_feasible->value.item<double>()) << "\n";
  }
  return 0;
}
```

Output

```text
exit flag 0, converged false
returned answer: objective 10.5147, violation 3.00243e-06
best feasible available: true
answer is the checkpoint: false
checkpoint point [0, 0]
checkpoint objective 18, violation 0
report matches checkpoint: true
```
Sequential QP was stopped after three iterations, so the flag is `MaxIterations` and `converged` is false. The returned answer is excellent in value (`10.5147`) but infeasible by `3e-06`, just over the default tolerance, because this method approaches feasibility from outside. The only feasible point it ever visited is the starting point, `(0, 0)` with objective `18`, so that is the checkpoint, and the answer in `params` is not that checkpoint (`false`). Here the checkpoint is feasible but far worse, which shows the trade-off: neither is automatically the one to use. A program that wants a usable point from a run that did not converge should read `best_feasible`, and check its violation, rather than `params`.
