<!-- generated from the_smallest_call.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The smallest call

[Back to Your First Constrained Problems](../optima_01_first_problems.md)


Start with no constraints at all. The objective is the squared distance to a target point, so the answer is the target. This shows the whole shape of a call: an objective written with `Trace`, a starting `Series`, and a result you read by block name.

```cpp
#include <pluss/ma.h>

#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::optim;
  const auto f64 = pluss::ScalarType::Float64;

  const auto target = pluss::make_series(std::vector<double>{1.0, 2.0}, {2}).to(f64);
  const auto start = pluss::make_series(std::vector<double>{0.0, 0.0}, {2}).to(f64);

  // f(x) = |x - target|^2, minimized where x == target.
  const auto result = optim::optima(
      [&](const optim::Trace& x) { return optim::sum(optim::square(x - optim::trace(target, false))); },
      start);

  const auto x = result.params.at("x").to_vec<double>();
  std::cout << "x = [" << x[0] << ", " << x[1] << "]\n";
  std::cout << "converged: " << std::boolalpha << result.converged << "\n";
  std::cout << "exit flag: " << static_cast<int>(result.exit_flag) << "\n";
  std::cout << "iterations: " << result.iteration << "\n";
  return 0;
}
```

Output

```text
x = [1, 2]
converged: true
exit flag: 1
iterations: 1
```
The solver reaches the target `(1, 2)` to within its tolerance, reports `converged: true`, and exits with flag `1`, which is the code for `Converged`. The answer is accurate to the stopping tolerance, not guaranteed to the last digit, so always judge a result against the tolerance rather than against an exact printed value.
