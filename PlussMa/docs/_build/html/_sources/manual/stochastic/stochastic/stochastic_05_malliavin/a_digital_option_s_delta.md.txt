<!-- generated from a_digital_option_s_delta.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# A digital option's delta

[Back to Malliavin Calculus](../stochastic_05_malliavin.md)


A digital option pays `1{X_T > K}`. Its pathwise derivative is zero almost everywhere, so the estimator of chapter 4 reports zero: the payoff has no slope except at a single point that no path hits. Integrating by parts with the Malliavin derivative puts the derivative on the weight instead. For geometric Brownian motion `delta = E[1{X_T > K} W_T] / (x₀ σ T)` (undiscounted), and the closed form is `n(d2) / (x₀ σ √T)`.

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace sde = pluss::subsp::math::sde;

int main() {
  const auto f64 = pluss::ScalarType::Float64;
  const double r = 0.02, sg = 0.3, x0 = 100.0, K = 100.0, T = 1.0;
  const std::int64_t paths = 400000;
  const pluss::Series y0 = pluss::Series::full({paths}, x0, f64);
  sde::SdeOptions o;
  o.steps = 1;  // GBM is solved exactly below from W_T, so one step of the scheme is not used
  o.seed = 77;
  const auto run = sde::fluctua([=](double, const pluss::Series& y, const pluss::Series&) { return y.mul(0.0); },
                                [=](double, const pluss::Series& y, const pluss::Series&) { return y.mul(0.0); }, y0, 0.0, T,
                                pluss::series::undefined(), o);
  const pluss::Series wT = run.wiener_final;  // the realized W_T of every path
  const pluss::Series xT = wT.mul(sg).add((r - 0.5 * sg * sg) * T).exp().mul(x0);
  const pluss::Series payoff = xT.gt(K).to(f64);

  // Pathwise: d payoff / d x0 = (derivative of the indicator) * (d X_T / d x0), and the indicator has slope zero
  // everywhere except at the single point X_T = K, which no path hits. Autodiff therefore has nothing to
  // propagate: a comparison is not a differentiable operation.
  std::cout << std::fixed << std::setprecision(5);
  std::cout << "pathwise estimate: 0 on every path\n";

  const double weighted = payoff.mul(wT).mean().item<double>() / (x0 * sg * T);
  const double d2 = (std::log(x0 / K) + (r - 0.5 * sg * sg) * T) / (sg * std::sqrt(T));
  const double closed = std::exp(-0.5 * d2 * d2) / std::sqrt(2 * M_PI) / (x0 * sg * std::sqrt(T));
  std::cout << "Malliavin-weight estimate  " << weighted << "\n";
  std::cout << "closed form n(d2)/(x0 sigma sqrt(T))  " << closed << "\n";
  return 0;
}
```

Output

```text
pathwise estimate: 0 on every path
Malliavin-weight estimate  0.01328
closed form n(d2)/(x0 sigma sqrt(T))  0.01325
```
The weighted estimate is 0.01328 against the closed form 0.01325, from 400000 paths, while the pathwise estimate is exactly zero. The weight is the whole trick: the payoff is untouched and so can be as discontinuous as you like.
