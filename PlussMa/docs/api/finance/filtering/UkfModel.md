<!-- generated from UkfModel.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `filtering::UkfModel`

A nonlinear state-space model for the unscented Kalman filter: the dynamics and the observation, without Jacobians.

The same model as [`EkfModel`](EkfModel.md) without the Jacobians, plus the sigma-point settings in `options` ([`UkfOptions`](UkfOptions.md)). The functions take the state `[n]` and the date index; the filter evaluates them at the sigma points and rebuilds the mean and covariance from the results, so it also copes with functions you cannot differentiate. The examples use a random walk observed through `h(x) = x + 0.2x²`.

## Fields

### `transition`

`filtering::StateFn`, default `empty`.

`f`, state `[n]` to `[n]`. Required.

### `observation`

`filtering::ObsFn`, default `empty`.

`h`, state `[n]` to `[m]`. Required.

### `process_cov`

`pluss::Series`, default `undefined`.

`Q`, `[n, n]`.

### `observation_cov`

`pluss::Series`, default `undefined`.

`R`, `[m, m]`.

### `initial_mean`

`pluss::Series`, default `undefined`.

`[n]`.

### `initial_cov`

`pluss::Series`, default `undefined`.

`[n, n]`.

### `info_tol`

`double`, default `0.0`.

Channels with forecast variance at most this are ignored.

### `options`

`filtering::UkfOptions`, default `{}`.

Sigma-point settings.

## Guidance

Passed to `ukf_filter`.

## Practical Examples

### Example 1

The transition and observation functions.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace flt = fin::filtering;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const std::int64_t T = 200;
  const double q = 0.01, r = 0.01;
  const Series x = (pluss::series::randn({T}, cfg, 1) * std::sqrt(q)).cumsum(0, false);
  const Series y = x + x.square() * 0.2 + pluss::series::randn({T}, cfg, 2) * std::sqrt(r);
  flt::UkfModel m;
  m.transition = [&](const Series &s, std::int64_t) { return s; };
  m.observation = [&](const Series &s, std::int64_t) { return s + s.square() * 0.2; };
  m.process_cov = pluss::series::full({1, 1}, q, cfg);
  m.observation_cov = pluss::series::full({1, 1}, r, cfg);
  m.initial_mean = pluss::series::zeros({1}, cfg);
  m.initial_cov = pluss::series::full({1, 1}, 1.0, cfg);

  const auto res = flt::ukf_filter(y, m, cfg);
  out.writeln("rms error of the filtered level:", (res.filtered_mean.reshape({T}) - x).square().mean().sqrt().item<double>(), "; raw observation:", (y - x).square().mean().sqrt().item<double>());
}
```

Output

```text
rms error of the filtered level: 0.0729 ; raw observation: 0.4701
```

### Example 2

A nondifferentiable observation: `h(x) = |x|` has no Jacobian at zero, but the unscented filter needs none. Here the sign is lost and only the magnitude is tracked.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace flt = fin::filtering;
using pluss::Series;

static double bs(fin::pricing::OptionType type, double S, double K, double r, double q, double T, double vol) {
  fin::module::Field f{.instrument = fin::instrument::make(type, K, T), .market = fin::market::make(S, r, q),
                       .model = fin::model::BlackScholes{.volatility = pluss::make_series({vol})},
                       .controls = {.engine = fin::module::Engine::Analytic, .risk = fin::module::Risk::PriceOnly}};
  return f.price().price.items<double>()[0];
}
static pluss::Series bsv(fin::pricing::OptionType type, const pluss::Series &spots, double K, double r, double q, double T, double vol) {
  fin::module::Field f{.instrument = fin::instrument::make(type, K, T), .market = {.spot = spots, .rate = pluss::make_series({r}), .dividend_yield = pluss::make_series({q})},
                       .model = fin::model::BlackScholes{.volatility = pluss::make_series({vol})},
                       .controls = {.engine = fin::module::Engine::Analytic, .risk = fin::module::Risk::PriceOnly}};
  return f.price().price;
}
static pluss::Series bst(fin::pricing::OptionType type, const pluss::Series &spots, const pluss::Series &tau, double K, double r, double q, double vol) {
  fin::module::Field f{.instrument = {.type = type, .strike = pluss::make_series({K}), .maturity = tau}, .market = {.spot = spots, .rate = pluss::make_series({r}), .dividend_yield = pluss::make_series({q})},
                       .model = fin::model::BlackScholes{.volatility = pluss::make_series({vol})},
                       .controls = {.engine = fin::module::Engine::Analytic, .risk = fin::module::Risk::PriceOnly}};
  return f.price().price;
}
static fin::hedging::DeltaHedgeResult hedge(const pluss::Series &path, fin::pricing::OptionType type, double K, double r, double q, double T, double vol) {
  fin::module::Field f{.instrument = fin::instrument::make(type, K, T), .market = fin::market::make(100.0, r, q),
                       .model = fin::model::BlackScholes{.volatility = pluss::make_series({vol})},
                       .controls = {.engine = fin::module::Engine::Analytic}};
  return fin::hedging::delta_hedge(f, path);
}
static double iv(double price, double S, double K, double r, double q, double T, const fin::pricing::ImpliedVolOptions &o = {}) {
  fin::module::Field f{.instrument = fin::instrument::make(fin::pricing::Call, K, T), .market = fin::market::make(S, r, q),
                       .model = fin::model::BlackScholes{.volatility = pluss::make_series({0.2})},
                       .controls = {.engine = fin::module::Engine::Analytic, .risk = fin::module::Risk::PriceOnly}};
  return f.implied_vol(pluss::make_series({price}), o).items<double>()[0];
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const std::int64_t T = 200;
  const double q = 0.01, r = 0.01;
  const Series x = (pluss::series::randn({T}, cfg, 1) * std::sqrt(q)).cumsum(0, false);
  const Series y = x + x.square() * 0.2 + pluss::series::randn({T}, cfg, 2) * std::sqrt(r);

  const Series ya = x.abs() + pluss::series::randn({T}, cfg, 7) * std::sqrt(r);
  flt::UkfModel m;
  m.transition = [&](const Series &s, std::int64_t) { return s; };
  m.observation = [&](const Series &s, std::int64_t) { return s.abs(); };
  m.process_cov = pluss::series::full({1, 1}, q, cfg);
  m.observation_cov = pluss::series::full({1, 1}, r, cfg);
  m.initial_mean = pluss::series::full({1}, 1.0, cfg);
  m.initial_cov = pluss::series::full({1, 1}, 1.0, cfg);
  const auto res = flt::ukf_filter(ya, m, cfg);
  out.writeln("rms error of |filtered level| against |x|:", (res.filtered_mean.reshape({T}).abs() - x.abs()).square().mean().sqrt().item<double>());
}
```

Output

```text
rms error of |filtered level| against |x|: 0.0695
```

### Example 3

Process and observation covariances.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace flt = fin::filtering;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const std::int64_t T = 200;
  const double q = 0.01, r = 0.01;
  const Series x = (pluss::series::randn({T}, cfg, 1) * std::sqrt(q)).cumsum(0, false);
  const Series y = x + x.square() * 0.2 + pluss::series::randn({T}, cfg, 2) * std::sqrt(r);
  flt::UkfModel m;
  m.transition = [&](const Series &s, std::int64_t) { return s; };
  m.observation = [&](const Series &s, std::int64_t) { return s + s.square() * 0.2; };
  m.process_cov = pluss::series::full({1, 1}, q, cfg);
  m.observation_cov = pluss::series::full({1, 1}, r, cfg);
  m.initial_mean = pluss::series::zeros({1}, cfg);
  m.initial_cov = pluss::series::full({1, 1}, 1.0, cfg);

  for (const double process : {1e-4, 0.01, 1.0}) {
    m.process_cov = pluss::series::full({1, 1}, process, cfg);
    const auto res = flt::ukf_filter(y, m, cfg);
    out.writeln("Q =", process, ": rms error", (res.filtered_mean.reshape({T}) - x).square().mean().sqrt().item<double>());
  }
}
```

Output

```text
Q = 0.0001 : rms error 0.1960
Q = 0.0100 : rms error 0.0729
Q = 1.0000 : rms error 0.2399
```

### Example 4

The initial mean and covariance. A very diffuse start is a trap on a nonlinear observation: with `P₀ = 100` the sigma points lie at ±10 where `h(x) = x + 0.2x²` is not monotone, and the first estimates are far off.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace flt = fin::filtering;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const std::int64_t T = 200;
  const double q = 0.01, r = 0.01;
  const Series x = (pluss::series::randn({T}, cfg, 1) * std::sqrt(q)).cumsum(0, false);
  const Series y = x + x.square() * 0.2 + pluss::series::randn({T}, cfg, 2) * std::sqrt(r);
  flt::UkfModel m;
  m.transition = [&](const Series &s, std::int64_t) { return s; };
  m.observation = [&](const Series &s, std::int64_t) { return s + s.square() * 0.2; };
  m.process_cov = pluss::series::full({1, 1}, q, cfg);
  m.observation_cov = pluss::series::full({1, 1}, r, cfg);
  m.initial_mean = pluss::series::zeros({1}, cfg);
  m.initial_cov = pluss::series::full({1, 1}, 1.0, cfg);
  m.initial_mean = pluss::series::full({1}, 3.0, cfg);

  for (const double p0 : {1e-4, 1.0, 100.0}) {
    m.initial_cov = pluss::series::full({1, 1}, p0, cfg);
    const auto res = flt::ukf_filter(y, m, cfg);
    out.writeln("P0 =", p0, ": filtered level at date 3 =", res.filtered_mean.select(0, 3).sum().item<double>(), "(truth", x.select(0, 3).item<double>(), ")");
  }
}
```

Output

```text
P0 = 0.000 : filtered level at date 3 = -0.101 (truth -0.124 )
P0 = 1.000 : filtered level at date 3 = -0.156 (truth -0.124 )
P0 = 100.000 : filtered level at date 3 = -2.499 (truth -0.124 )
```

### Example 5

The information tolerance.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace flt = fin::filtering;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(0));
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const std::int64_t T = 200;
  const double q = 0.01, r = 0.01;
  const Series x = (pluss::series::randn({T}, cfg, 1) * std::sqrt(q)).cumsum(0, false);
  const Series y = x + x.square() * 0.2 + pluss::series::randn({T}, cfg, 2) * std::sqrt(r);
  flt::UkfModel m;
  m.transition = [&](const Series &s, std::int64_t) { return s; };
  m.observation = [&](const Series &s, std::int64_t) { return s + s.square() * 0.2; };
  m.process_cov = pluss::series::full({1, 1}, q, cfg);
  m.observation_cov = pluss::series::full({1, 1}, r, cfg);
  m.initial_mean = pluss::series::zeros({1}, cfg);
  m.initial_cov = pluss::series::full({1, 1}, 1.0, cfg);

  for (const double tol : {0.0, 1e6}) {
    m.info_tol = tol;
    out.writeln("info_tol", tol, ": observations used", flt::ukf_filter(y, m, cfg).data_used.sum().item<double>(), "of", T);
  }
}
```

Output

```text
info_tol 0 : observations used 200 of 200
info_tol 1000000 : observations used 0 of 200
```

### Example 6

The sigma-point options are part of the model.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace flt = fin::filtering;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const std::int64_t T = 200;
  const double q = 0.01, r = 0.01;
  const Series x = (pluss::series::randn({T}, cfg, 1) * std::sqrt(q)).cumsum(0, false);
  const Series y = x + x.square() * 0.2 + pluss::series::randn({T}, cfg, 2) * std::sqrt(r);
  flt::UkfModel m;
  m.transition = [&](const Series &s, std::int64_t) { return s; };
  m.observation = [&](const Series &s, std::int64_t) { return s + s.square() * 0.2; };
  m.process_cov = pluss::series::full({1, 1}, q, cfg);
  m.observation_cov = pluss::series::full({1, 1}, r, cfg);
  m.initial_mean = pluss::series::zeros({1}, cfg);
  m.initial_cov = pluss::series::full({1, 1}, 1.0, cfg);
  m.options.alpha = 0.5;

  out.writeln("alpha set to", m.options.alpha, "; rms error", (flt::ukf_filter(y, m, cfg).filtered_mean.reshape({T}) - x).square().mean().sqrt().item<double>());
}
```

Output

```text
alpha set to 0.5000 ; rms error 0.0729
```

## See Also

- [`ukf_filter`](ukf_filter.md): uses it
- [`UkfOptions`](UkfOptions.md): the sigma-point settings
- [`EkfModel`](EkfModel.md): with Jacobians

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ParticleResampleTrigger.html">
    <span class="label">Previous</span>
    <span class="title">ParticleResampleTrigger</span>
  </a>
  <a class="next-link" href="UkfOptions.html">
    <span class="label">Next</span>
    <span class="title">UkfOptions</span>
  </a>
</div>
<!-- apiref:nav:end -->
