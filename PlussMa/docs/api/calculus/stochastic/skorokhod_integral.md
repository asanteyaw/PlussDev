<!-- generated from skorokhod_integral.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `operators::stochastic::skorokhod_integral`

The Skorokhod integral of a vector process on a discretised Wiener path: an Itô integral that also accepts anticipating integrands.

## Signature

```cpp
operators::Operator operators::stochastic::skorokhod_integral(pluss::StackArray<std::string_view> path, double dt, operators::DiffOptions options = {});
```

## What It Does

The Itô integral requires the integrand at each step not to depend on the future of the path. The *Skorokhod integral* removes that restriction: it is the adjoint of the Malliavin derivative, and for a vector `u = (u₁, …, u_N)` (one component per path step) on a path with step `dt` it is

`δ(u) = Σᵢ uᵢ dWᵢ − dt Σᵢ D_{tᵢ}uᵢ`,

the Itô sum minus a correction involving the Malliavin derivatives. For an *adapted* integrand (each `uᵢ` depends only on earlier levels) the correction vanishes and `δ(u)` is the Itô sum. For an anticipating integrand such as `uᵢ = W_N` (every component reads the final value) it differs: `δ = W_N² − N dt`, whereas the Itô-style sum `Σ W_N dWᵢ = W_N²`.

`skorokhod_integral({"w1", …, "wN"}, dt)` applied to a rank-1 function of the levels with `N` components returns the scalar function `δ(u)`. It takes `N²` differentials, so use it on short paths. The sampled versions for long paths remain on `Series::calc`.

## Parameters

`pluss::StackArray<std::string_view> path`
: The path levels `W₁ … W_N`, in order.

`double dt`
: The time step of the discretisation, positive.

`operators::DiffOptions options`
: Options for the derivatives.

## Returns

An `Operator` whose application returns `δ(u)` as a scalar `Function` of the path levels.

## Errors

- `needs a non-empty path and a positive step`.
- `u must have one component per path step`.

## Practical Examples

### Example 1

An anticipating integrand. With `uᵢ = W_N` for every step, `δ(u) = W_N² − N dt`. For the path `W = (0.3, −0.2, 0.5, 0.9)`, `N = 4`, `dt = 0.25`: `0.81 − 1 = −0.19`.

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
  out.print_defaults(PrintOptions{}.fixed(6));
  const math::Function u = math::function({"w1", "w2", "w3", "w4"}, [](math::Function, math::Function, math::Function, math::Function d) { return math::stack({d, d, d, d}); });
  const std::vector<pluss::Series> w = {pluss::make_series(0.3), pluss::make_series(-0.2), pluss::make_series(0.5), pluss::make_series(0.9)};
  out.writeln("delta(u) =", stochastic::skorokhod_integral({"w1", "w2", "w3", "w4"}, 0.25)(u)(pluss::RangeView<pluss::Series>(w)).item<double>(), "   W_N^2 - N dt =", 0.9 * 0.9 - 4 * 0.25);
}
```

Output

```text
delta(u) = -0.190000    W_N^2 - N dt = -0.190000
```

### Example 2

An adapted integrand agrees with the Itô sum. With `uᵢ = Wᵢ₋₁` (the previous level; `u₁ = 0`) the correction vanishes, and `δ(u) = Σ Wᵢ₋₁ (Wᵢ − Wᵢ₋₁) = −0.09` on the same path.

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
  out.print_defaults(PrintOptions{}.fixed(6));
  const math::Function u = math::function({"w1", "w2", "w3", "w4"}, [](math::Function w1, math::Function w2, math::Function w3, math::Function) { return math::stack({w1 * 0.0, w1, w2, w3}); });
  const std::vector<pluss::Series> w = {pluss::make_series(0.3), pluss::make_series(-0.2), pluss::make_series(0.5), pluss::make_series(0.9)};
  const double ito_sum = 0.0 * 0.3 + 0.3 * (-0.2 - 0.3) + (-0.2) * (0.5 + 0.2) + 0.5 * (0.9 - 0.5);
  out.writeln("delta(u) =", stochastic::skorokhod_integral({"w1", "w2", "w3", "w4"}, 0.25)(u)(pluss::RangeView<pluss::Series>(w)).item<double>(), "   Ito sum =", ito_sum);
}
```

Output

```text
delta(u) = -0.090000    Ito sum = -0.090000
```

### Example 3

A deterministic integrand has no Malliavin derivative, so the Skorokhod integral is the Wiener integral `Σ cᵢ dWᵢ`: for `c = (1, 2, 3, 4)` on the same path, `1·0.3 + 2·(−0.5) + 3·0.7 + 4·0.4 = 3.0`.

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
  out.print_defaults(PrintOptions{}.fixed(6));
  const math::Function c = math::function({"w1", "w2", "w3", "w4"}, [](math::Function w1, math::Function, math::Function, math::Function) { return math::stack({w1 * 0.0 + 1.0, w1 * 0.0 + 2.0, w1 * 0.0 + 3.0, w1 * 0.0 + 4.0}); });
  const std::vector<pluss::Series> w = {pluss::make_series(0.3), pluss::make_series(-0.2), pluss::make_series(0.5), pluss::make_series(0.9)};
  out.writeln("delta(c) =", stochastic::skorokhod_integral({"w1", "w2", "w3", "w4"}, 0.25)(c)(pluss::RangeView<pluss::Series>(w)).item<double>(), "   sum c_i dW_i =", 1 * 0.3 + 2 * (-0.5) + 3 * 0.7 + 4 * 0.4);
}
```

Output

```text
delta(c) = 3.000000    sum c_i dW_i = 3.000000
```

### Example 4

The path must be non-empty with a positive step, and the integrand must have one component per step.

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

  const math::Function scalar = math::function({"w1", "w2"}, [](math::Function a, math::Function) { return a; });
  try { (void)stochastic::skorokhod_integral({"w1", "w2"}, -1.0); } catch (const pluss::Error &e) { out.writeln("negative step:  ", first_line(e)); }
  try { (void)stochastic::skorokhod_integral({"w1", "w2"}, 0.5)(scalar); } catch (const pluss::Error &e) { out.writeln("scalar integrand:", first_line(e)); }
}
```

Output

```text
negative step:   operators::stochastic::skorokhod_integral: needs a non-empty path and a positive step
scalar integrand: operators::stochastic::skorokhod_integral: u must have one component per path step
```

## See Also

- [`malliavin_derivative`](malliavin_derivative.md): its adjoint
- [`stochastic_integral`](stochastic_integral.md): the Itô integral along a sampled path

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="quadratic_variation.html">
    <span class="label">Previous</span>
    <span class="title">quadratic_variation</span>
  </a>
  <a class="next-link" href="stochastic_differential.html">
    <span class="label">Next</span>
    <span class="title">stochastic_differential</span>
  </a>
</div>
<!-- apiref:nav:end -->
