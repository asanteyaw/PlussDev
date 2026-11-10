<!-- generated from unconditional_variance.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `volatility::unconditional_variance`

The long-run variance of an NGARCH or GARCH pricing model.

## Signature

```cpp
double volatility::unconditional_variance(const volatility::Ngarch11Params &params);
double volatility::unconditional_variance(const volatility::Garch11Params &params);
```

## What It Does

`ω/(1 − β − α(1 + γ²))` for the [`Ngarch11Params`](Ngarch11Params.md) and `ω/(1 − α − β)` for the [`Garch11Params`](Garch11Params.md): the level the variance reverts to. It is finite only for a stationary parameter set; a non-stationary one is refused. Use it to check the units of a fit (a daily model with long-run variance 0.4 in percent-squared is an annual volatility of `√(252·0.4)` percent) and as the starting variance of a simulation.

## Parameters

`const volatility::Ngarch11Params &params`
: NGARCH parameters.

`const volatility::Garch11Params &params`
: GARCH parameters.

## Returns

The long-run variance.

## Errors

- A non-stationary set or invalid parameters throw (`pluss::Error`).

## Practical Examples

### Example 1

A GARCH(1,1): `0.02/(1 − 0.98) = 1.0`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  out.writeln(fin::volatility::unconditional_variance(fin::volatility::Garch11Params{.omega = 0.02, .alpha = 0.08, .beta = 0.90}));
}
```

Output

```text
1.0000
```

### Example 2

An NGARCH with leverage: the persistence is `β + α(1 + γ²)`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  out.writeln(fin::volatility::unconditional_variance(fin::volatility::Ngarch11Params{.omega = 0.02, .alpha = 0.05, .beta = 0.85, .gamma = 1.0}));
}
```

Output

```text
0.4000
```

### Example 3

From a fit: the long-run variance of a fitted GARCH agrees with the fit's own.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  ts::GjrFit truth;
  truth.mean = 0.04; truth.omega = 0.03; truth.arch = {0.03}; truth.asym = {0.12}; truth.garch = {0.88};
  const Series returns = truth.simulate(1500, pluss::Config{}.with_dtype(pluss::ScalarType::Float64), 11);
  const auto garch = ts::Garch::fit_mle(returns, {}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));
  const auto gjr = ts::Gjr::fit_mle(returns, {}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));

  const auto p = fin::volatility::to_pricing_params(garch);
  out.writeln("from the bridge:", fin::volatility::unconditional_variance(p), ", from the fit:", garch.unconditional_var());
}
```

Output

```text
from the bridge: 0.54454 , from the fit: 0.54454
```

### Example 4

A non-stationary set is refused.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const fin::volatility::Garch11Params p{.omega = 0.02, .alpha = 0.2, .beta = 0.85};
  try {
    (void)(fin::volatility::unconditional_variance(p));
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find('\n')));
  }
}
```

Output

```text
subsp::math::finance::unconditional_variance: requires beta + alpha*(1+gamma^2) < 1 - stationarity_margin
```

## See Also

- [`Ngarch11Params`](Ngarch11Params.md): the model
- [`Garch11Params`](Garch11Params.md): the model

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_pricing_params.html">
    <span class="label">Previous</span>
    <span class="title">to_pricing_params</span>
  </a>
  <a class="next-link" href="variance_forecast.html">
    <span class="label">Next</span>
    <span class="title">variance_forecast</span>
  </a>
</div>
<!-- apiref:nav:end -->
