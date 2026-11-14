<!-- generated from measures.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Integration against a measure

[Back to Operators](../calculus_05_operators.md)

There is no measure type. A measure is carried by the operator that integrates against it, and measure algebra is the operator algebra you already have: a mixture is `linear_combination`, a change of measure is `multiply` by the Radon–Nikodym density, a product measure is `compose` of two integrals. Each operator returns an analytic `Function` of the remaining variables.

| Entry point | Measure |
|---|---|
| `measure::lebesgue(x, a, b, null_points)` | `dμ = dx` on `[a, b]`; null points are never sampled |
| `measure::weighted(x, density, a, b)` | `dμ = ρ dx` |
| `measure::atomic(x, points, weights)` | `Σ wᵢ δ_{xᵢ}`: the exact sum `Σ wᵢ f(xᵢ)` |
| `measure::stieltjes(x, cdf, a, b, jump_points, jump_sizes)` | `dF`: the density of the continuous part plus explicit jumps |

```cpp
operators::Operator operators::measure::lebesgue(std::string_view variable, double lower, double upper, RangeView<double> null_points = {});
operators::Operator operators::measure::weighted(std::string_view variable, const math::Function &density, double lower, double upper, RangeView<double> null_points = {});
operators::Operator operators::measure::atomic(std::string_view variable, RangeView<double> points, RangeView<double> weights);
operators::Operator operators::measure::stieltjes(std::string_view variable, const math::Function &cdf, double lower, double upper, RangeView<double> jump_points = {}, RangeView<double> jump_sizes = {});
```

Atomic measures are exact finite sums; the others are integrals evaluated by quadrature when you supply a value. They save and load except `weighted` and `stieltjes`, which hold a function.

## Expectations, mixtures and a change of measure

The second moment of a standard normal is 1. A mixture `½·Lebesgue[0,1] + 2·δ₁` integrates `x²` to `½/3 + 2`. Weighting Lebesgue measure on `[0, 1]` by the density `3x²` and integrating `x` gives `3/4`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
namespace measure = pluss::operators::measure;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const math::Function square = math::function({"x"}, [](math::Function x) { return x * x; });
  const math::Function identity = math::function({"x"}, [](math::Function x) { return x; });
  const math::Function pdf = math::function({"x"}, [](math::Function x) { return (-(x * x) * 0.5).exp() / std::sqrt(2 * std::acos(-1.0)); });
  const math::Function density = math::function({"x"}, [](math::Function x) { return x * x * 3.0; });

  out.writeln("E[x^2], normal       ", measure::weighted("x", pdf, -10.0, 10.0)(square)().item<double>());

  const operators::Operator mixture = operators::linear_combination({{0.5, measure::lebesgue("x", 0.0, 1.0)}, {1.0, measure::atomic("x", {1.0}, {2.0})}});
  out.writeln("mixture of x^2       ", mixture(square)().item<double>(), "   exact ", 0.5 / 3 + 2.0);

  const operators::Operator tilted = operators::compose(measure::lebesgue("x", 0.0, 1.0), operators::multiply(density));
  out.writeln("change of measure    ", tilted(identity)().item<double>(), "   exact 0.75");

  const math::Function xy = math::function({"x", "y"}, [](math::Function x, math::Function y) { return x * y; });
  const operators::Operator product = operators::compose(measure::lebesgue("y", 0.0, 1.0), measure::lebesgue("x", 0.0, 2.0));
  out.writeln("product measure      ", product(xy)().item<double>(), "   exact 1");
}
```

Output

```text
E[x^2], normal        1.00000000
mixture of x^2        2.16666667    exact  2.16666667
change of measure     0.75000000    exact 0.75
product measure       1.00000000    exact 1
```

## Range partition on arrays

The Lebesgue integral partitions the range of the integrand instead of the domain. That evaluator is eager and works on arrays, so it lives with the numerical integrals rather than the operators: `integ::lebesgue(f, a, b, options)` returns the value with a certified `lower`/`upper` bracket and the refinement history, and `integ::measure_of(f, a, b, level)` returns `μ({x : f(x) > level})`. Grading the cells (`options.grading`) handles endpoint singularities such as `1/√x`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace integ = pluss::subsp::math::integ;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  integ::LebesgueOptions options;
  options.grading = 4.0;
  const auto r = integ::lebesgue([](const pluss::Series &x) { return x.rsqrt(); }, 0.0, 1.0, options);
  out.writeln("int x^-1/2 = ", r.value.item<double>(), "  bracket [", r.lower.item<double>(), ", ", r.upper.item<double>(), "]  converged ", r.converged);
  const auto mu = integ::measure_of([](const pluss::Series &x) { return x.square(); }, 0.0, 1.0, 0.36);
  out.writeln("mu(x^2 > 0.36) = ", mu.value.item<double>(), "   exact 0.4");
}
```

Output

```text
int x^-1/2 =  2.00000000   bracket [ 2.00000000 ,  2.00000000 ]  converged  true
mu(x^2 > 0.36) =  0.40000000    exact 0.4
```

<!-- apiref:toctree:start -->
<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="integral_transforms.html">
    <span class="label">Previous</span>
    <span class="title">Fourier and Laplace transforms</span>
  </a>
  <a class="next-link" href="vector_variational_and_adjoint.html">
    <span class="label">Next</span>
    <span class="title">Vector, variational and adjoint operators</span>
  </a>
</div>
<!-- apiref:nav:end -->
