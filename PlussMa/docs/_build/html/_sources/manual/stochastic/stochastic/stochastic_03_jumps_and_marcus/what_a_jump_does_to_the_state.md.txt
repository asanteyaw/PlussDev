<!-- generated from what_a_jump_does_to_the_state.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# What a jump does to the state

[Back to Jumps and the Marcus Integral](../stochastic_03_jumps_and_marcus.md)


A jump of size `z` under coefficient `h` moves the state along the flow of `dy/du = h(y) z`. Three coefficients have closed-form flows: `h = 1` (additive), `h = y` (multiplicative, so `log y` moves by exactly `z`) and `h = y(1 − y)` (logistic, which keeps the state inside (0, 1)).

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

int main() {
  const auto f64 = pluss::ScalarType::Float64;
  const pluss::Series y = pluss::make_series(std::vector<double>{0.2, 0.5, 0.9}, {3}).to(f64);
  const pluss::Series z = pluss::make_series(std::vector<double>{1.5, -2.0, 3.0}, {3}).to(f64);
  const std::vector<double> yv = {0.2, 0.5, 0.9}, zv = {1.5, -2.0, 3.0};

  const auto add = y.calc.diff.marcus_flow([](const pluss::Series& s) { return s.mul(0.0).add(1.0); }, z).host<double>();
  const auto mul = y.calc.diff.marcus_flow([](const pluss::Series& s) { return s; }, z).host<double>();
  const auto logi = y.calc.diff.marcus_flow([](const pluss::Series& s) { return s.mul(s.neg().add(1.0)); }, z).host<double>();

  std::cout << std::fixed << std::setprecision(6);
  for (int i = 0; i < 3; ++i) {
    std::cout << "y = " << yv[i] << ", z = " << zv[i] << ":  additive " << add[i] << " (" << yv[i] + zv[i] << ")  multiplicative "
              << mul[i] << " (" << yv[i] * std::exp(zv[i]) << ")  logistic " << logi[i] << " ("
              << yv[i] / (yv[i] + (1 - yv[i]) * std::exp(-zv[i])) << ")\n";
  }
  return 0;
}
```

Output

```text
y = 0.200000, z = 1.500000:  additive 1.700000 (1.700000)  multiplicative 0.896338 (0.896338)  logistic 0.528396 (0.528396)
y = 0.500000, z = -2.000000:  additive -1.500000 (-1.500000)  multiplicative 0.067668 (0.067668)  logistic 0.119203 (0.119203)
y = 0.900000, z = 3.000000:  additive 3.900000 (3.900000)  multiplicative 18.076951 (18.076983)  logistic 0.994499 (0.994499)
```
Every row matches the closed form. The additive flow adds `z`, and the logistic flow never leaves (0, 1): from 0.9 with `z = 3` it ends at 0.994499. The flow is followed by a Runge-Kutta scheme with 32 steps by default; the multiplicative jump from 0.9 with `z = 3` is 18.076951 against the exact 18.076983, and the `flow_steps` option raises the accuracy when the jumps are large.
