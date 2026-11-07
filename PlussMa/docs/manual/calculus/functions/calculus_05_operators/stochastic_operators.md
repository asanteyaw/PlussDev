<!-- generated from stochastic_operators.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Stochastic operators

[Back to Operators](../calculus_05_operators.md)

Some operators act on a function of a random path rather than on a function of numbers. They live in `operators::stochastic`, they are ordinary `Operator` values, and they share the rules of every other operator: applying one gives a `Function`, which is then evaluated. What is different is what the arguments mean. A **path functional** is evaluated on arrays that hold the samples of a sample path along one axis (the *path axis*, the last one unless you say otherwise); every other axis is an independent path, and the result has the path axis summed out.

The stochastic integral is never a mode of the ordinary integral. `integral("x", a, b)` keeps its meaning when its argument happens to be random; to integrate against a noise you call `stochastic_integral` and say which convention you mean.

| Operator | Meaning | Kind |
|---|---|---|
| `stochastic_integral(noise, options)` | the sum of the integrand, taken at `(1 − θ)` times the left and `θ` times the right end of each step, times the noise increments | path functional |
| `quadratic_variation(options)`, `covariation(other, options)` | sums of squared and of crossed increments | path functional |
| `generator(drift, diffusion, states)` | the infinitesimal generator `L g = μ·∇g + ½ tr(σσᵀ ∇²g)` | structural, linear |
| `stochastic_differential(drift, diffusion, states, convention, θ)` | the drift and diffusion of `g(X)` in the same convention | structural, linear |
| `convention_change(θ_from, θ_to, diffusion, states)` | the drift of the same process written in another convention | structural, linear |

`Convention` is `Ito` (`θ = 0`, the integrand cannot anticipate the increment it multiplies), `Stratonovich` (`θ = ½`, the ordinary chain rule holds), `Klimontovich` (`θ = 1`) or `Theta` with your own `θ` in `[0, 1]`.

```cpp
operators::Operator operators::stochastic::stochastic_integral(std::string_view noise, IntegralOptions options = {});
operators::Operator operators::stochastic::quadratic_variation(PathOptions options = {});
operators::Operator operators::stochastic::covariation(math::Function other, PathOptions options = {});
operators::Operator operators::stochastic::generator(math::Function drift, math::Function diffusion, StackArray<std::string_view> states, DiffOptions options = {});
operators::Operator operators::stochastic::stochastic_differential(math::Function drift, math::Function diffusion, StackArray<std::string_view> states, Convention convention = Convention::Ito, double theta = 0.0, DiffOptions options = {});
operators::Operator operators::stochastic::convention_change(double theta_from, double theta_to, math::Function diffusion, StackArray<std::string_view> states, DiffOptions options = {});
```

## Integrals along a path

Because the integrand is a `Function` of the sampled variables, it is a pointwise formula and cannot look ahead along the path: the Itô sum is non-anticipating by construction. An opaque callable, which could look ahead, is rejected when the operator is applied.

For `∫ W dW` the three conventions have exact discrete values on any sampled path. With `ΔW` the increments and `[W] = Σ (ΔW)²` the quadratic variation, the Itô sum is `(W_T² − [W])/2`, the Stratonovich sum is `W_T²/2` and the Klimontovich sum is `(W_T² + [W])/2`. The Itô and Stratonovich values differ by exactly half the quadratic variation, which is not an error of the scheme but the difference between the two integrals.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
namespace stochastic = pluss::operators::stochastic;
namespace sde = pluss::subsp::math::sde;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const std::int64_t steps = 2000;
  const sde::WienerPath path = sde::simulate_wiener_path(pluss::make_series(0.0, as_f64()), 0.0, 1.0, steps, 7);
  const pluss::Series times = pluss::make_series(path.times, {steps + 1});

  const math::Function integrand = math::function({"t", "W"}, [](math::Function, math::Function w) { return w; });
  const auto value = [&](stochastic::IntegralOptions options) {
    return stochastic::stochastic_integral("W", options)(integrand)({times, path.w}).item<double>();
  };
  const double ito = value({});
  const double strat = value({.convention = stochastic::Convention::Stratonovich});
  const double klim = value({.convention = stochastic::Convention::Klimontovich});
  const double qv = stochastic::quadratic_variation()(integrand)({times, path.w}).item<double>();
  const double w_final = path.w.select(0, steps).item<double>();

  out.writeln("Ito                 ", ito, "   (W_T^2 - [W]) / 2 = ", (w_final * w_final - qv) / 2.0);
  out.writeln("Stratonovich        ", strat, "   W_T^2 / 2         = ", w_final * w_final / 2.0);
  out.writeln("Klimontovich        ", klim, "   (W_T^2 + [W]) / 2 = ", (w_final * w_final + qv) / 2.0);
  out.writeln("[W] (about 1)       ", qv);
  out.writeln("Strat - Ito         ", strat - ito, "   [W] / 2 = ", qv / 2.0);
}
```

Output

```text
Ito                  -0.48047324    (W_T^2 - [W]) / 2 =  -0.48047324
Stratonovich         0.03459461    W_T^2 / 2         =  0.03459461
Klimontovich         0.54966246    (W_T^2 + [W]) / 2 =  0.54966246
[W] (about 1)        1.03013571
Strat - Ito          0.51506785    [W] / 2 =  0.51506785
```

The three sums reproduce their closed forms to rounding, the quadratic variation of a standard Wiener path over one unit of time is close to 1, and the Stratonovich minus Itô gap is half of it.

### Malliavin derivative and Skorokhod integral

On a discretized Wiener path whose levels `W₁ … W_N` are the variables of a `Function`, the Malliavin derivative and the Skorokhod integral are exact formulas. Shifting the increments `dW_i` by `h_i dt` moves `W_j` by `dt Σ_{i≤j} h_i`, so

- `malliavin_derivative({W1, …, WN})` maps a scalar functional `F` to the vector `D_{t_i}F = Σ_{j≥i} ∂F/∂W_j`;
- `skorokhod_integral({W1, …, WN}, dt)` maps a vector `u` with `N` components to `δ(u) = Σ u_i dW_i − dt Σ_i D_{t_i}u_i`, which is the Itô sum when `u` is adapted and `W_N² − N dt` for `u_i = W_N`.

Both are built from `differential`, so they differentiate and save like any `Function`; the Skorokhod integral takes `N²` differentials, so use it on short paths. The sampled versions for long paths stay on `Series::calc`, as does the Marcus integral.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

namespace math = pluss::math;
namespace stochastic = pluss::operators::stochastic;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const auto path = [](auto body) { return math::function({"w1", "w2", "w3", "w4"}, body); };
  const std::vector<pluss::Series> w{pluss::make_series(0.3), pluss::make_series(-0.2), pluss::make_series(0.5), pluss::make_series(0.9)};

  const math::Function F = path([](math::Function, math::Function, math::Function c, math::Function) { return c.exp(); });   // exp(W_3)
  out.writeln("D exp(W_3)      ", stochastic::malliavin_derivative({"w1", "w2", "w3", "w4"})(F)(w).pull<double>(), "   exp(0.5) = ", std::exp(0.5));

  const math::Function u = path([](math::Function, math::Function, math::Function, math::Function d) { return math::stack({d, d, d, d}); });
  const math::Function delta = stochastic::skorokhod_integral({"w1", "w2", "w3", "w4"}, 0.25)(u);
  out.writeln("delta(W_N)      ", delta(w).item<double>(), "   W_N^2 - N dt = ", 0.9 * 0.9 - 4 * 0.25);
}
```

Output

```text
D exp(W_3)       [1.648721, 1.648721, 1.648721, 0.000000]    exp(0.5) =  1.648721
delta(W_N)       -0.190000    W_N^2 - N dt =  -0.190000
```

## Generators, Itô's formula and changes of convention

These three operators are deterministic calculus on a `Function`: no path is sampled. For `dX = μ dt + σ ∘_θ dW` the infinitesimal generator `L` applies to a test function, and `stochastic_differential` returns the drift and diffusion of `g(X)` in the same convention, `a = g′μ + (½ − θ) g″σ²` and `b = g′σ`: Itô's formula at `θ = 0`, the ordinary chain rule at `θ = ½`, the backward rule at `θ = 1`. `convention_change` moves a drift from one reading to another, `μ + (θ_from − θ_to) σσ′`. For several states the drift is a vector, the diffusion a matrix, and the formulas use the gradient and the Hessian.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

namespace math = pluss::math;
namespace stochastic = pluss::operators::stochastic;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  // geometric Brownian motion dX = a X dt + b X dW
  const double a = 0.3, b = 0.5, x = 1.4;
  const math::Function drift = math::function({"x"}, [&](math::Function v) { return v * a; });
  const math::Function diffusion = math::function({"x"}, [&](math::Function v) { return v * b; });
  const math::Function square = math::function({"x"}, [](math::Function v) { return v * v; });
  const auto point = pluss::make_series(x, as_f64());

  const auto coefficients = [&](stochastic::Convention c) {
    return stochastic::stochastic_differential(drift, diffusion, {"x"}, c)(square)({point}).pull<double>();
  };
  const auto ito = coefficients(stochastic::Convention::Ito);
  const auto strat = coefficients(stochastic::Convention::Stratonovich);
  out.writeln("d(X^2), Ito:           drift ", ito[0], "  diffusion ", ito[1], "   2 a x^2 + b^2 x^2 = ", 2 * a * x * x + b * b * x * x);
  out.writeln("d(X^2), Stratonovich:  drift ", strat[0], "  diffusion ", strat[1], "   2 a x^2           = ", 2 * a * x * x);

  // the generator of the same process applied to x^3: (3a + 3 b^2) x^3
  const math::Function cube = math::function({"x"}, [](math::Function v) { return v.pow(3.0); });
  const auto L = stochastic::generator(drift, diffusion, {"x"});
  out.writeln("L x^3                  ", L(cube)({point}).item<double>(), "   (3a + 3b^2) x^3 = ", (3 * a + 3 * b * b) * x * x * x);

  // the drift of the Stratonovich reading written as an Ito drift: a x + b^2 x / 2
  const math::Function ito_drift = stochastic::convention_change(0.5, 0.0, diffusion, {"x"})(drift);
  out.writeln("Ito drift at x         ", ito_drift({point}).item<double>(), "   a x + b^2 x / 2 = ", a * x + 0.5 * b * b * x);
}
```

Output

```text
d(X^2), Ito:           drift  1.666000   diffusion  1.960000    2 a x^2 + b^2 x^2 =  1.666000
d(X^2), Stratonovich:  drift  1.176000   diffusion  1.960000    2 a x^2           =  1.176000
L x^3                   4.527600    (3a + 3b^2) x^3 =  4.527600
Ito drift at x          0.595000    a x + b^2 x / 2 =  0.595000
```

## What can be saved

`stochastic_integral` and `quadratic_variation` hold only names and numbers and can be saved with `math::io::save` like the built-in operators. `generator`, `stochastic_differential`, `convention_change` and `covariation` hold `Function`s and cannot ([Saving and loading](saving_and_loading.md)). Every `Function` these operators produce is an analytic graph and saves like any other.

<!-- apiref:toctree:start -->
<!-- apiref:toctree:end -->
