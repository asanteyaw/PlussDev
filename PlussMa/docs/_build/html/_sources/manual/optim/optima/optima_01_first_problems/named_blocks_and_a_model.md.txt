<!-- generated from named_blocks_and_a_model.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Named blocks and a model

[Back to Your First Constrained Problems](../optima_01_first_problems.md)


Named blocks shine when the unknowns mean something. This program fits a straight line to five points. The objective reads like the model, with `slope` and `intercept` as separate blocks, and the answer is read back by those names.

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

  const auto t = optim::trace(vec({0.0, 1.0, 2.0, 3.0, 4.0}), false);
  const auto y = optim::trace(vec({1.1, 2.9, 5.2, 7.1, 8.8}), false);

  auto sum_squared_error = [&](const Pack& p) {
    const auto prediction = p.at("slope") * t + p.at("intercept");
    return optim::sum(optim::square(prediction - y));
  };

  const auto start = optim::pack_scalars({{"slope", pluss::make_series(0.0).to(f64)},
                                          {"intercept", pluss::make_series(0.0).to(f64)}});
  const auto result = optim::optima(sum_squared_error, start);

  std::cout << std::fixed << std::setprecision(4);
  std::cout << "slope     = " << result.params.at("slope").item<double>() << "\n";
  std::cout << "intercept = " << result.params.at("intercept").item<double>() << "\n";
  std::cout << "blocks returned: " << result.params.size() << "\n";
  return 0;
}
```

Output

```text
slope     = 1.9600
intercept = 1.1000
blocks returned: 2
```
The slope `1.9601` and intercept `1.0999` match the closed-form least-squares line for these data, `1.96` and `1.10`, to the solver tolerance. The result keeps both blocks, in order.
