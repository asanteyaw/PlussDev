<!-- generated from strat_integral.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `sde::strat_integral`

The Stratonovich integral `∫ f ∘ dW`, with the integrand averaged over both ends of each step.

## Signature

```cpp
sde::StochasticIntegralResult sde::strat_integral(const sde::WienerIntegrandFn &f, const sde::WienerPath &path);
sde::StochasticIntegralResult sde::strat_integral(const sde::SampledIntegrandFn &make, const sde::WienerPath &path, const sde::AdaptednessOptions &check = {});
```

## What It Does

The Stratonovich integral is the limit of the trapezoidal sums `Σ ½[f(tᵢ, W(tᵢ)) + f(tᵢ₊₁, W(tᵢ₊₁))] (W(tᵢ₊₁) − W(tᵢ))`. Its virtue is that it obeys the **ordinary chain rule** (`∫ W ∘ dW = W(T)²/2`), which is why physical models with smooth noise as a limit are naturally Stratonovich. Its cost is that it is not a martingale and does not preserve zero mean.

For the integrand `f = W` the Stratonovich integral exceeds the Itô one by exactly half the realised quadratic variation of the path, `QV/2`, which tends to `T/2`. This is not discretisation error and it does not shrink with the step. In general the gap is `½ ∫ ∂f/∂w dt`. The two signatures mirror [`ito_integral`](ito_integral.md): a pointwise integrand, or one computed from the sampled path and checked for adaptedness.

## Parameters

`const sde::WienerIntegrandFn &f`
: `Series (double t, const Series &w)`, averaged over both ends of each step.

`const sde::SampledIntegrandFn &make`
: `Series (const Series &path)`, checked for adaptedness.

`const sde::WienerPath &path`
: The path to integrate along.

`const sde::AdaptednessOptions &check`
: Settings of the look-ahead check.

## Returns

A [`StochasticIntegralResult`](StochasticIntegralResult.md).

## Errors

- As for `ito_integral`.

## Practical Examples

### Example 1

The chain rule. `∫₀ᵀ W ∘ dW = W(T)²/2`: no `−T/2` correction.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace sde = pluss::subsp::math::sde;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const Series like = pluss::make_series(0.0).to(pluss::ScalarType::Float64).reshape({1});

  for (const std::int64_t steps : {50, 500, 5000}) {
    const sde::WienerPath path = sde::simulate_wiener_path(like, 0.0, 1.0, steps, 6);
    const auto r = sde::strat_integral([](double, const Series &w) { return w; }, path);
    const double wT = path.w.select(0, steps).sum().item<double>();
    out.writeln("steps", steps, ": integral =", r.value.sum().item<double>(), ", W^2/2 =", 0.5 * wT * wT);
  }
}
```

Output

```text
steps 50 : integral = 0.00449 , W^2/2 = 0.00449
steps 500 : integral = 0.05325 , W^2/2 = 0.05325
steps 5000 : integral = 0.42259 , W^2/2 = 0.42259
```

### Example 2

Ito and Stratonovich on one path. Their difference is exactly half the realised quadratic variation, at every step count.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace sde = pluss::subsp::math::sde;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const Series like = pluss::make_series(0.0).to(pluss::ScalarType::Float64).reshape({1});

  for (const std::int64_t steps : {20, 200, 2000}) {
    const sde::WienerPath path = sde::simulate_wiener_path(like, 0.0, 1.0, steps, 7);
    const auto ito = sde::ito_integral([](double, const Series &w) { return w; }, path);
    const auto strat = sde::strat_integral([](double, const Series &w) { return w; }, path);
    out.writeln("steps", steps, ": strat - ito =", (strat.value - ito.value).sum().item<double>(), ", QV / 2 =", 0.5 * ito.realized_quadratic_variation.sum().item<double>());
  }
}
```

Output

```text
steps 20 : strat - ito = 0.38570382 , QV / 2 = 0.38570382
steps 200 : strat - ito = 0.44949219 , QV / 2 = 0.44949219
steps 2000 : strat - ito = 0.51506785 , QV / 2 = 0.51506785
```

### Example 3

The gap depends on the integrand. For a constant integrand there is no gap; for `f = W²` it is `½ ∫ 2W dt`, so it is path-dependent.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace sde = pluss::subsp::math::sde;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const Series like = pluss::make_series(0.0).to(pluss::ScalarType::Float64).reshape({1});
  const sde::WienerPath path = sde::simulate_wiener_path(like, 0.0, 1.0, 4000, 3);

  for (const bool constant : {true, false}) {
    const sde::WienerIntegrandFn f = [constant](double, const Series &w) { return constant ? w * 0.0 + 2.0 : w * w; };
    const auto ito = sde::ito_integral(f, path);
    const auto strat = sde::strat_integral(f, path);
    out.writeln(constant ? "f = 2  " : "f = W^2", ": strat - ito =", (strat.value - ito.value).sum().item<double>());
  }
}
```

Output

```text
f = 2   : strat - ito = 0.00000
f = W^2 : strat - ito = 0.40962
```

### Example 4

From the sampled path: the running maximum is adapted, so the Stratonovich sum of it is computed.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace sde = pluss::subsp::math::sde;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const Series like = pluss::make_series(0.0).to(pluss::ScalarType::Float64).reshape({1});
  const sde::WienerPath path = sde::simulate_wiener_path(like, 0.0, 1.0, 300, 9);

  auto running_max = [](const Series &w) {
    Series best = w.select(0, 0);
    Series result = best.unsqueeze(0);
    for (std::int64_t i = 1; i < w.size(0); ++i) {
      best = best.maximum(w.select(0, i));
      result = result.unite({best.unsqueeze(0)}, 0);
    }
    return result;
  };
  const auto r = sde::strat_integral(sde::SampledIntegrandFn{running_max}, path);
  out.writeln("value =", r.value.items<double>());
}
```

Output

```text
value = [-0.88216]
```

### Example 5

From the sampled path: an integrand that reads the final value of the path anticipates and is refused.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>

using namespace pluss::tils;

namespace sde = pluss::subsp::math::sde;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const Series like = pluss::make_series(0.0).to(pluss::ScalarType::Float64).reshape({1});
  const sde::WienerPath path = sde::simulate_wiener_path(like, 0.0, 1.0, 300, 9);

  auto terminal = [](const Series &w) { return w.select(0, w.size(0) - 1).unsqueeze(0).expand({w.size(0), w.size(1)}); };
  try {
    (void)sde::strat_integral(sde::SampledIntegrandFn{terminal}, path);
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find('\n')));
  }
}
```

Output

```text
subsp::math::sde::strat_integral: the integrand at sample 0 changed when only samples after sample 60 of the path were redrawn, so it looks into the future of the path (it is not adapted) and the sum is not an Ito integral
```

### Example 6

A missing integrand is rejected.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>

using namespace pluss::tils;

namespace sde = pluss::subsp::math::sde;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const Series like = pluss::make_series(0.0).to(pluss::ScalarType::Float64).reshape({1});
  const sde::WienerPath path = sde::simulate_wiener_path(like, 0.0, 1.0, 10, 3);

  try {
    (void)sde::strat_integral(sde::WienerIntegrandFn{}, path);
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find('\n')));
  }
}
```

Output

```text
subsp::math::sde::strat_integral: integrand callback is required
```

## See Also

- [`StochasticIntegralResult`](StochasticIntegralResult.md): what it returns
- [`WienerPath`](WienerPath.md): the path
- [`ito_integral`](ito_integral.md): the Itô convention

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="simulate_wiener_path.html">
    <span class="label">Previous</span>
    <span class="title">simulate_wiener_path</span>
  </a>
</div>
<!-- apiref:nav:end -->
