<!-- generated from bounds_that_do_not_match_the_blocks.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Bounds that do not match the blocks

[Back to Your First Constrained Problems](../optima_01_first_problems.md)


`lbs` and `ubs` with a plain vector attach to a block named `x`. On a problem whose blocks have other names, `optima` refuses the problem before it starts and shows both layouts, so you can see the mismatch at once. The fix is to build bounds as a pack with the same blocks.

```cpp
#include <pluss/ma.h>

#include <iostream>
#include <string>

int main() {
  namespace optim = pluss::optim;
  const auto f64 = pluss::ScalarType::Float64;
  using Pack = optim::ParameterPack<optim::Trace>;
  auto scalar = [&](double v) { return pluss::make_series(v).to(f64); };

  auto objective = [](const Pack& p) { return optim::square(p.at("a")) + optim::square(p.at("b")); };
  const auto start = optim::pack_scalars({{"a", scalar(1.0)}, {"b", scalar(1.0)}});

  try {
    const auto wrong = optim::constraints().lbs(pluss::make_series(std::vector<double>{0.0, 0.0}, {2}).to(f64));
    (void)optim::optima(objective, start, {}, wrong);
  } catch (const std::exception& e) {
    std::string message = e.what();
    std::cout << "refused: " << message.substr(0, message.find('\n')) << "\n";
  }

  const auto right = optim::constraints().lbs(optim::pack_scalars({{"a", scalar(0.5)}, {"b", scalar(0.0)}}));
  const auto result = optim::optima(objective, start, {}, right);
  std::cout << "with matching blocks: a = " << result.params.at("a").item<double>() << ", b = "
            << result.params.at("b").item<double>() << "\n";
  return 0;
}
```

Output

```text
refused: optim::evaluate_problem: lower bounds must have the same named blocks and shapes as the parameters (parameters: a[1], b[1]; lower bounds: x[2])
with matching blocks: a = 0.5, b = 1.20218e-10
```
The first call is refused with a message naming the parameter blocks and the bounds layout. The second succeeds once the bounds carry the block names `a` and `b`.
