<!-- generated from an_objective_that_returns_more_than_one_number.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# An objective that returns more than one number

[Back to Your First Constrained Problems](../optima_01_first_problems.md)


The objective must reduce to a single number. Returning a vector of residuals, instead of their sum of squares, is the usual slip. The solver returns immediately with the `InvalidProblem` flag and a message that names the fix.

```cpp
#include <pluss/ma.h>

#include <iostream>

int main() {
  namespace optim = pluss::optim;
  const auto start = pluss::make_series(std::vector<double>{1.0, 1.0}, {2}).to(pluss::ScalarType::Float64);

  const auto result = optim::optima([](const optim::Trace& x) { return optim::square(x); }, start);

  std::cout << "exit flag: " << static_cast<int>(result.exit_flag) << "\n";
  std::cout << "converged: " << std::boolalpha << result.converged << "\n";
  std::string message = result.message;
  std::cout << "message: " << message.substr(0, message.find('\n')) << "\n";
  return 0;
}
```

Output

```text
exit flag: -5
converged: false
message: objective autodiff callback failed: optim: the objective must return a single number, but it returned 2 values; reduce it with sum() or mean()
```
`optima` does not throw here: it returns a result with exit flag `-5` and an explanation, so a calling program can inspect it. `converged` is false because the exit flag is not `Converged`.
