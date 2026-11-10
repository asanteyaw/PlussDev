<!-- generated from malliavin_derivative.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `operators::stochastic::malliavin_derivative`

The Malliavin derivative of a functional of a discretised Wiener path, as a vector of sensitivities to each increment.

## Signature

```cpp
operators::Operator operators::stochastic::malliavin_derivative(pluss::StackArray<std::string_view> path, operators::DiffOptions options = {});
```

## What It Does

A random variable that depends on a Brownian path, such as the payoff of an option, can be differentiated with respect to the path itself. The *Malliavin derivative* `D_{tᵢ}F` measures how `F` changes when the increment `dWᵢ` is shifted: on a path discretised at levels `W₁ … W_N` (with `W₀ = 0`, so `dWᵢ = Wᵢ − Wᵢ₋₁`), shifting `dWᵢ` by `h dt` moves every later level, so

`D_{tᵢ}F = ∂F/∂(dWᵢ) = Σ_{j ≥ i} ∂F/∂Wⱼ`.

`malliavin_derivative({"w1", …, "wN"})` is the operator that takes a scalar function `F` of the levels and returns the vector `[D_{t₁}F, …, D_{t_N}F]`. It is exact for the discretised functional, built from `differential`, so it can be evaluated on any path. Malliavin derivatives give sensitivities of expectations without differentiating the payoff (integration by parts on Wiener space), and they underlie the Skorokhod integral ([`skorokhod_integral`](skorokhod_integral.md)). The sampled versions for long paths remain on `Series::calc`.

## Parameters

`pluss::StackArray<std::string_view> path`
: The variables `W₁ … W_N`, the levels of the discretised path, in order.

`operators::DiffOptions options`
: Options for the derivatives.

## Returns

An `Operator` whose application returns the vector `[D_{t₁}F, …, D_{t_N}F]` as a rank-1 `Function` of length `N`.

## Errors

- `the path needs at least one variable`.
- `the functional must be scalar-valued`.

## Practical Examples

### Example 1

`F = exp(W₃)` on a 4-level path. It depends on the third level only, so `D_{tᵢ}F = e^{W₃}` for `i ≤ 3` and 0 for `i = 4`: at `W₃ = 0.5` that is `1.64872` three times, then 0.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <vector>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace stochastic = pluss::operators::stochastic;
namespace sde = pluss::subsp::math::sde;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const math::Function F = math::function({"w1", "w2", "w3", "w4"}, [](math::Function, math::Function, math::Function c, math::Function) { return c.exp(); });
  const std::vector<pluss::Series> w = {pluss::make_series(0.3), pluss::make_series(-0.2), pluss::make_series(0.5), pluss::make_series(0.9)};
  out.writeln("D exp(W_3) =", stochastic::malliavin_derivative({"w1", "w2", "w3", "w4"})(F)(pluss::RangeView<pluss::Series>(w)).items<double>(), "   exp(0.5) =", std::exp(0.5));
}
```

Output

```text
D exp(W_3) = [1.64872, 1.64872, 1.64872, 0.00000]    exp(0.5) = 1.64872
```

### Example 2

The chain rule. For `F = W_N²` the derivative with respect to every increment is `2 W_N` (each increment moves the last level one-for-one): the vector is `2W_N` repeated.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <vector>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace stochastic = pluss::operators::stochastic;
namespace sde = pluss::subsp::math::sde;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const math::Function F = math::function({"w1", "w2", "w3", "w4"}, [](math::Function, math::Function, math::Function, math::Function d) { return d * d; });
  const std::vector<pluss::Series> w = {pluss::make_series(0.3), pluss::make_series(-0.2), pluss::make_series(0.5), pluss::make_series(0.9)};
  out.writeln("D W_N^2 =", stochastic::malliavin_derivative({"w1", "w2", "w3", "w4"})(F)(pluss::RangeView<pluss::Series>(w)).items<double>(), "   2 W_N =", 1.8);
}
```

Output

```text
D W_N^2 = [1.80000, 1.80000, 1.80000, 1.80000]    2 W_N = 1.80000
```

### Example 3

A running maximum-like functional, `F = (W₁ + W₂ + W₃ + W₄)/4` (the average of the path): moving the first increment moves all four levels, the last only one, so `D = (4, 3, 2, 1)/4`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <vector>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace stochastic = pluss::operators::stochastic;
namespace sde = pluss::subsp::math::sde;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const math::Function F = math::function({"w1", "w2", "w3", "w4"}, [](math::Function a, math::Function b, math::Function c, math::Function d) { return (a + b + c + d) * 0.25; });
  const std::vector<pluss::Series> w = {pluss::make_series(0.3), pluss::make_series(-0.2), pluss::make_series(0.5), pluss::make_series(0.9)};
  out.writeln("D (average) =", stochastic::malliavin_derivative({"w1", "w2", "w3", "w4"})(F)(pluss::RangeView<pluss::Series>(w)).items<double>(), "  (4, 3, 2, 1) / 4");
}
```

Output

```text
D (average) = [1.00000, 0.75000, 0.50000, 0.25000]   (4, 3, 2, 1) / 4
```

### Example 4

The path needs at least one variable and the functional must be scalar-valued.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace stochastic = pluss::operators::stochastic;
namespace sde = pluss::subsp::math::sde;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto first_line = [](const pluss::Error &e) {
    const std::string text(e.what());
    return text.substr(0, text.find('\n'));
  };

  const math::Function vector_valued = math::function({"w1", "w2"}, [](math::Function a, math::Function b) { return math::stack({a, b}); });
  try { (void)stochastic::malliavin_derivative({}); } catch (const pluss::Error &e) { out.writeln("empty path:      ", first_line(e)); }
  try { (void)stochastic::malliavin_derivative({"w1", "w2"})(vector_valued); } catch (const pluss::Error &e) { out.writeln("vector functional:", first_line(e)); }
}
```

Output

```text
empty path:       operators::stochastic::malliavin_derivative: the path needs at least one variable
vector functional: operators::stochastic::malliavin_derivative: the functional must be scalar-valued
```

## See Also

- [`skorokhod_integral`](skorokhod_integral.md): the adjoint of the Malliavin derivative
- [`differential`](../differential/differential.md): what it is built from

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="generator.html">
    <span class="label">Previous</span>
    <span class="title">generator</span>
  </a>
  <a class="next-link" href="quadratic_variation.html">
    <span class="label">Next</span>
    <span class="title">quadratic_variation</span>
  </a>
</div>
<!-- apiref:nav:end -->
