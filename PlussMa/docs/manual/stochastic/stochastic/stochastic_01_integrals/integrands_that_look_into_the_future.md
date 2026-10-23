<!-- generated from integrands_that_look_into_the_future.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Integrands that look into the future

[Back to Stochastic Integrals](../stochastic_01_integrals.md)


An Ito integral is only meaningful for an adapted integrand, one whose value at a time depends on the path up to that time and no further. A callback can break that without anything visible going wrong. The wrapper tests the callback: it redraws the part of the path after a chosen sample and demands that the integrand's earlier values do not change.

```cpp
#include <pluss/ma.h>

#include <iostream>
#include <string>
#include <vector>

namespace sde = pluss::subsp::math::sde;
using pluss::Series;

int main() {
  const auto f64 = pluss::ScalarType::Float64;
  const Series like = pluss::make_series(0.0).to(f64).reshape({1});
  const auto path = sde::simulate_wiener_path(like, 0.0, 1.0, 200, /*seed=*/9);

  // adapted: the running maximum of the path up to each sample
  auto running_max = [](const Series& w) {
    Series best = w.select(0, 0);
    Series out = best.unsqueeze(0);
    for (std::int64_t i = 1; i < w.size(0); ++i) {
      best = best.maximum(w.select(0, i));
      out = out.unite({best.unsqueeze(0)}, 0);
    }
    return out;
  };
  // anticipating: average of the previous, current and next sample
  auto centered_average = [](const Series& w) {
    const std::int64_t n = w.size(0);
    const Series previous = w.slice(0, 0, 1, 1).unite({w.slice(0, 0, n - 1, 1)}, 0);
    const Series next = w.slice(0, 1, n, 1).unite({w.slice(0, n - 1, n, 1)}, 0);
    return previous.add(w).add(next).div(3.0);
  };
  // anticipating: the value the path ends at
  auto terminal_value = [](const Series& w) {
    return w.select(0, w.size(0) - 1).unsqueeze(0).expand({w.size(0), w.size(1)});
  };

  struct Case { const char* label; sde::SampledIntegrandFn make; };
  const std::vector<Case> cases = {{"running maximum", running_max},
                                   {"centered moving average", centered_average},
                                   {"terminal value", terminal_value}};
  for (const auto& c : cases) {
    try {
      const auto r = sde::ito_integral(c.make, path);
      std::cout << c.label << ": accepted, Ito integral = " << r.value.item<double>() << "\n";
    } catch (const std::exception& e) {
      std::string message = e.what();
      message = message.substr(0, message.find('\n'));
      std::cout << c.label << ": rejected\n    " << message << "\n";
    }
  }
  return 0;
}
```

Output

```text
running maximum: accepted, Ito integral = 0.555349
centered moving average: rejected
    subsp::math::sde::ito_integral: the integrand at sample 40 changed when only samples after sample 40 of the path were redrawn, so it looks into the future of the path (it is not adapted) and the sum is not an Ito integral
terminal value: rejected
    subsp::math::sde::ito_integral: the integrand at sample 0 changed when only samples after sample 40 of the path were redrawn, so it looks into the future of the path (it is not adapted) and the sum is not an Ito integral
```
The running maximum is adapted and accepted. A centered moving average reads samples after the current one, and the terminal value reads the last sample from the first: both are rejected, and the message says at which sample the integrand changed. The test detects any dependence on the path itself. It cannot detect state a callback captured from outside, for example a precomputed copy of the whole path, so passing the test is evidence, not proof.
