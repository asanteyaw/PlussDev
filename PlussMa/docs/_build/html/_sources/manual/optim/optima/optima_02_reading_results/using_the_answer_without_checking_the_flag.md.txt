<!-- generated from using_the_answer_without_checking_the_flag.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Using the answer without checking the flag

[Back to Reading Results and Watching a Run](../optima_02_reading_results.md)


The result always holds a point. After a failure that point is just wherever the solver ended. Using it as if it were a solution is the most common mistake in code that calls a solver.

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
  // x0^2 + x1^2 <= -1 has no solution.
  const auto impossible = optim::constraints().nl_ineq(
      [](const Pack& p) { return optim::sum(optim::square(p.at("x"))) + 1.0; });

  const auto r = optim::optima(objective, vec({0.0, 0.0}), {}, impossible);

  const auto x = r.params.at("x").to_vec<double>();
  std::cout << "unchecked answer: [" << x[0] << ", " << x[1] << "]  (looks like a result)\n";
  std::cout << "converged: " << std::boolalpha << r.converged << ", exit flag " << static_cast<int>(r.exit_flag)
            << ", violation " << r.evaluation.max_constraint_violation << "\n";
  std::cout << "best feasible point exists: " << r.output.best_feasible_available << "\n";
  if (r.exit_flag != optim::OptimaExitFlag::Converged) {
    std::cout << "decision: do not use this answer\n";
  }
  return 0;
}
```

Output

```text
unchecked answer: [0, 0]  (looks like a result)
converged: false, exit flag -6, violation 1
best feasible point exists: false
decision: do not use this answer
```
The impossible problem returns the start point, `[0, 0]`, which looks like a perfectly good answer. Only the flag (`-6`) and the violation (`1`) reveal that it is not. Check the flag before reading `params`.
