<!-- generated from sensitivity_of_a_probability.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Worked problem: the sensitivity of a probability

A normal variable with standard deviation σ falls in [−1, 1] with probability

P(σ) = ∫₋₁¹ (σ√(2π))⁻¹ exp(−x²/(2σ²)) dx = erf(1/(σ√2)).

The integral has no closed form in the rule table, so `integral` evaluates it numerically. The question is how P changes with σ: the derivative dP/dσ. By Leibniz's rule that is the integral of the derivative of the density with respect to σ, which `differential` constructs and which is again a numerical integral. The closed form, for checking, is dP/dσ = −2 exp(−1/(2σ²)) / (σ²√(2π)).

The program builds the density as a function of `x` and `sigma`, integrates it over [−1, 1], differentiates with respect to `sigma`, and evaluates both for five standard deviations in single calls.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

#include <cmath>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  constexpr double two_pi = 6.283185307179586;
  const math::Function density = math::function({"x", "sigma"}, [two_pi](math::Function x, math::Function sigma) {
    return (-(x * x) / (sigma * sigma * 2.0)).exp() / (sigma * std::sqrt(two_pi));
  });

  const math::Function probability = operators::integral("x", -1.0, 1.0)(density);
  const math::Function sensitivity = operators::differential("sigma")(probability);

  const auto sigma = pluss::make_series({0.25, 0.5, 1.0, 2.0, 4.0});
  const auto p = probability({sigma}).pull<double>();
  const auto dp = sensitivity({sigma}).pull<double>();

  std::vector<double> exact_p, exact_dp;
  for (const double s : sigma.pull<double>()) {
    exact_p.push_back(std::erf(1.0 / (s * std::sqrt(2.0))));
    exact_dp.push_back(-2.0 * std::exp(-1.0 / (2.0 * s * s)) / (s * s * std::sqrt(two_pi)));
  }
  out.writeln("sigma      ", sigma.pull<double>());
  out.writeln("P(sigma)   ", p);
  out.writeln("erf form   ", exact_p);
  out.writeln("dP/dsigma  ", dp);
  out.writeln("closed form", exact_dp);
  out.writeln("provenance: P", probability.provenance() == math::Provenance::Quadrature ? "quadrature" : "exact", " dP/dsigma",
              sensitivity.provenance() == math::Provenance::Quadrature ? "quadrature" : "exact");
}
```

Output

```text
sigma       [0.25000000, 0.50000000, 1.00000000, 2.00000000, 4.00000000]
P(sigma)    [0.99993666, 0.95449974, 0.68268949, 0.38292492, 0.19741265]
erf form    [0.99993666, 0.95449974, 0.68268949, 0.38292492, 0.19741265]
dP/dsigma   [-0.00428257, -0.43192773, -0.48394145, -0.17603266, -0.04833351]
closed form [-0.00428257, -0.43192773, -0.48394145, -0.17603266, -0.04833351]
provenance: P quadrature  dP/dsigma quadrature
```

The probability agrees with the error function form at all five standard deviations to every printed digit, and so does its derivative with respect to σ. The signs and sizes make sense: widening the distribution lowers the probability of landing in [−1, 1], and the sensitivity is largest in magnitude near σ ≈ 0.6, where the density at the boundary is changing fastest, and small at both extremes. Both quantities are `Quadrature` functions evaluated at five parameter values in one call each. The derivative was obtained without any hand-derived formula and without differencing: `differential` differentiated the density symbolically and wrapped it in the integral.

<!-- apiref:toctree:start -->
<!-- apiref:toctree:end -->
