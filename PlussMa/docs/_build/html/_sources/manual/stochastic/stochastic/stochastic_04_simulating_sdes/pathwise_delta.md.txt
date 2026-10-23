<!-- generated from pathwise_delta.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Pathwise delta

[Back to Simulating and Differentiating SDEs](../stochastic_04_simulating_sdes.md)


For a call with payoff `max(X_T − K, 0)`, differentiating through the simulated paths gives `1{X_T > K} X_T / x₀` per path, whose mean is `e^(rT) N(d1)`. `fluctua_adjoint` takes the loss as a function of the final state and returns `d loss / d y0` for every path.

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace sde = pluss::subsp::math::sde;

int main() {
  const auto f64 = pluss::ScalarType::Float64;
  const double r = 0.03, sg = 0.25, x0 = 100.0, K = 105.0, T = 1.0;
  const std::int64_t paths = 100000;
  const pluss::Series y0 = pluss::Series::full({paths}, x0, f64);
  sde::SdeOptions o;
  o.method = sde::SdeMethod::Milstein;
  o.steps = 64;
  o.seed = 21;

  const auto adj = sde::fluctua_adjoint(
      [=](double, const pluss::Series& y, const pluss::Series&) { return y.mul(r); },
      [=](double, const pluss::Series& y, const pluss::Series&) { return y.mul(sg); }, y0, 0.0, T, pluss::series::undefined(),
      [=](const pluss::Series& yT) { return yT.sub(K).relu().sum().mul(1.0 / static_cast<double>(paths)); }, o);

  const double delta_mc = adj.grad_y0.sum().item<double>();  // loss was the mean, so summing the per-path gradients gives the mean derivative
  const double d1 = (std::log(x0 / K) + (r + 0.5 * sg * sg) * T) / (sg * std::sqrt(T));
  const double delta_bs = std::exp(r * T) * 0.5 * std::erfc(-d1 / std::sqrt(2.0));
  std::cout << std::fixed << std::setprecision(4);
  std::cout << "pathwise delta (adjoint)      " << delta_mc << "\n";
  std::cout << "closed form e^(rT) N(d1)      " << delta_bs << "\n";
  const pluss::Series per_path = adj.grad_y0.mul(static_cast<double>(paths));  // the loss was a mean: undo the 1/N
  const double se = std::sqrt(per_path.sub(delta_mc).pow(2.0).mean().item<double>() / static_cast<double>(paths));
  std::cout << "standard error of the estimate  " << se << "\n";
  return 0;
}
```

Output

```text
pathwise delta (adjoint)      0.5329
closed form e^(rT) N(d1)      0.5357
standard error of the estimate  0.0020
```
The estimate is 0.5329 and the closed form 0.5357. They differ by 0.0028, 1.4 standard errors, where the program prints the standard error 0.0020 of the 100000-path estimate. The loss is any scalar function of the final state, differentiated by the same engine, so a smooth payoff needs no hand-derived gradient.
