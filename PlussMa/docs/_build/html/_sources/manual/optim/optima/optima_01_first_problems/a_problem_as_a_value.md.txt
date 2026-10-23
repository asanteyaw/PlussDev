<!-- generated from a_problem_as_a_value.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# A problem as a value

[Back to Your First Constrained Problems](../optima_01_first_problems.md)


When the same problem is solved from several starting points, or handed to another function, bundle it into a `ConstrainedProblem`. You can fill the struct directly or use the `problem()` builder, which accepts the same callables as the struct and picks the right slot. Either value is passed to `optima` with a start given as a plain vector or as a pack.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::optim;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  using Pack = optim::ParameterPack<optim::Trace>;
  const auto target = optim::trace(vec({1.0, 2.0}), false);
  auto objective = [target](const Pack& p) { return optim::sum(optim::square(p.at("x") - target)); };
  auto budget = [](const Pack& p) { return optim::sum(p.at("x")) - 1.5; };  // x0 + x1 <= 1.5

  // Route 1: fill the struct.
  optim::ConstrainedProblem by_hand;
  by_hand.objective.autodiff = objective;
  by_hand.nonlinear.inequality = budget;

  // Route 2: the builder.
  const optim::ConstrainedProblem built = optim::problem().objective(objective).nl_ineq(budget).build();

  const auto start = vec({0.0, 0.0});
  const auto a = optim::optima(by_hand, start);                           // start as a plain vector
  const auto b = optim::optima(built, optim::pack_series(start));        // start as a named-block pack

  const auto xa = a.params.at("x").to_vec<double>();
  const auto xb = b.params.at("x").to_vec<double>();
  std::cout << std::fixed << std::setprecision(4);
  std::cout << "struct:  x = [" << xa[0] << ", " << xa[1] << "]\n";
  std::cout << "builder: x = [" << xb[0] << ", " << xb[1] << "]\n";
  std::cout << "same answer: " << std::boolalpha << (xa == xb) << "\n";
  std::cout << "pack start returns block names: " << b.params.contains("x") << "\n";
  return 0;
}
```

Output

```text
struct:  x = [0.2500, 1.2500]
builder: x = [0.2500, 1.2500]
same answer: true
pack start returns block names: true
```
Both routes produce the same answer, the point of `x0 + x1 <= 1.5` nearest `(1, 2)`. Starting from a plain vector produces a block named `x`, which is the same result as passing the vector wrapped as a pack.
