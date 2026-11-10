<!-- generated from returns_metrics.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `bridge::returns_metrics`

The log-likelihood and conditional variances of returns under a fitted GARCH, EGARCH or GJR model.

## Signature

```cpp
bridge::ReturnsMetrics bridge::returns_metrics(const pluss::Series &returns, const stats::ts::GarchFit &fit, pluss::Config cfg = {});
bridge::ReturnsMetrics bridge::returns_metrics(const pluss::Series &returns, const stats::ts::EgarchFit &fit, pluss::Config cfg = {});
bridge::ReturnsMetrics bridge::returns_metrics(const pluss::Series &returns, const stats::ts::GjrFit &fit, pluss::Config cfg = {});
```

## What It Does

Evaluates a fitted model on a return sample (the sample it was fitted to, or new data) and returns its Gaussian log-likelihood, the per-observation terms and the conditional variances. The three overloads take the GARCH, the exponential GARCH and the Glosten-Jagannathan-Runkle fits; the numbers are the fit's own `loglik` and `infer`. Because the models are nested (GJR contains GARCH), twice the difference of log-likelihoods is a likelihood-ratio statistic for the asymmetry. Returns are a vector, one per period, in the unit of the fit; the variance is in that unit squared per period.

## Parameters

`const pluss::Series &returns`
: Returns, one per period.

`const stats::ts::GarchFit &fit / EgarchFit / GjrFit`
: A fitted model.

`pluss::Config cfg`
: Device.

## Returns

A [`ReturnsMetrics`](ReturnsMetrics.md).

## Errors

- Undefined, empty or non-finite returns throw (`pluss::Error`).

## Practical Examples

### Example 1

A GJR against a GARCH on data with a real asymmetry: the likelihood-ratio statistic for the extra parameter.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  ts::GjrFit truth;
  truth.mean = 0.04; truth.omega = 0.03; truth.arch = {0.03}; truth.asym = {0.12}; truth.garch = {0.88};
  const Series returns = truth.simulate(1500, pluss::Config{}.with_dtype(pluss::ScalarType::Float64), 11);
  const auto garch = ts::Garch::fit_mle(returns, {}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));
  const auto gjr = ts::Gjr::fit_mle(returns, {}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);

  const auto g = fin::bridge::returns_metrics(returns, garch, cfg), j = fin::bridge::returns_metrics(returns, gjr, cfg);
  out.writeln("GARCH loglik", g.loglik, ", GJR loglik", j.loglik, ", 2 x difference", 2.0 * (j.loglik - g.loglik), "(5% point of chi-squared(1): 3.84)");
}
```

Output

```text
GARCH loglik -1606.995 , GJR loglik -1595.536 , 2 x difference 22.917 (5% point of chi-squared(1): 3.84)
```

### Example 2

An EGARCH model, evaluated directly (parameters set by hand rather than fitted).

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  ts::GjrFit truth;
  truth.mean = 0.04; truth.omega = 0.03; truth.arch = {0.03}; truth.asym = {0.12}; truth.garch = {0.88};
  const Series returns = truth.simulate(1500, pluss::Config{}.with_dtype(pluss::ScalarType::Float64), 11);
  ts::EgarchFit egarch;
  egarch.mean = 0.04; egarch.omega = -0.1; egarch.arch = {0.15}; egarch.asym = {-0.05}; egarch.garch = {0.95};
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);

  const auto m = fin::bridge::returns_metrics(returns, egarch, cfg);
  out.writeln("loglik", m.loglik, ", mean conditional variance", m.variance.mean().item<double>());
}
```

Output

```text
loglik -1729.23512728 , mean conditional variance 0.31771178
```

### Example 3

New data: a model fitted on the first half evaluated on the second half.

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
  ts::GjrFit truth;
  truth.mean = 0.04; truth.omega = 0.03; truth.arch = {0.03}; truth.asym = {0.12}; truth.garch = {0.88};
  const Series returns = truth.simulate(1500, pluss::Config{}.with_dtype(pluss::ScalarType::Float64), 11);
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);

  const auto fit = ts::Garch::fit_mle(returns.slice(0, 0, 750, 1), {}, cfg);
  const auto out_of_sample = fin::bridge::returns_metrics(returns.slice(0, 750, 1500, 1), fit, cfg);
  const auto in_sample = fin::bridge::returns_metrics(returns.slice(0, 0, 750, 1), fit, cfg);
  out.writeln("log-likelihood per observation: in sample", in_sample.loglik / 750.0, ", out of sample", out_of_sample.loglik / 750.0);
}
```

Output

```text
log-likelihood per observation: in sample -1.0935 , out of sample -1.0521
```

### Example 4

The variance and the likelihood agree with the fit's own methods.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace ts = pluss::subsp::stats::ts;
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
  out.print_defaults(PrintOptions{}.fixed(10));
  ts::GjrFit truth;
  truth.mean = 0.04; truth.omega = 0.03; truth.arch = {0.03}; truth.asym = {0.12}; truth.garch = {0.88};
  const Series returns = truth.simulate(1500, pluss::Config{}.with_dtype(pluss::ScalarType::Float64), 11);
  const auto garch = ts::Garch::fit_mle(returns, {}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));
  const auto gjr = ts::Gjr::fit_mle(returns, {}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);

  const auto m = fin::bridge::returns_metrics(returns, garch, cfg);
  out.writeln("loglik difference", m.loglik - garch.loglik(returns), ", variance difference", (m.variance - garch.infer(returns, cfg).reshape({1500})).abs().max().item<double>());
}
```

Output

```text
loglik difference 0.0000000000 , variance difference 0.0000000000
```

### Example 5

The returns must be a vector of finite numbers.

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
  ts::GjrFit truth;
  truth.mean = 0.04; truth.omega = 0.03; truth.arch = {0.03}; truth.asym = {0.12}; truth.garch = {0.88};
  const Series returns = truth.simulate(1500, pluss::Config{}.with_dtype(pluss::ScalarType::Float64), 11);
  const auto garch = ts::Garch::fit_mle(returns, {}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));
  const auto gjr = ts::Gjr::fit_mle(returns, {}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));

  try {
    (void)(fin::bridge::returns_metrics(pluss::Series{}, garch));
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find('\n')));
  }
}
```

Output

```text
subsp::stats::ts::GarchFit::loglik: expected defined Series
```

### Example 6

Asymmetry in the variances: after the worst day of the sample the GJR variance jumps more than the GARCH one (the leverage effect).

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  ts::GjrFit truth;
  truth.mean = 0.04; truth.omega = 0.03; truth.arch = {0.03}; truth.asym = {0.12}; truth.garch = {0.88};
  const Series returns = truth.simulate(1500, pluss::Config{}.with_dtype(pluss::ScalarType::Float64), 11);
  const auto garch = ts::Garch::fit_mle(returns, {}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));
  const auto gjr = ts::Gjr::fit_mle(returns, {}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);

  const auto g = fin::bridge::returns_metrics(returns, garch, cfg), j = fin::bridge::returns_metrics(returns, gjr, cfg);
  const std::int64_t worst = returns.argmin().item<std::int64_t>();
  out.writeln("return", returns.select(0, worst).item<double>(), "; GARCH variance before/after", g.variance.select(0, worst).item<double>(), g.variance.select(0, worst + 1).item<double>(), "; GJR", j.variance.select(0, worst).item<double>(), j.variance.select(0, worst + 1).item<double>());
}
```

Output

```text
return -2.733 ; GARCH variance before/after 1.275 1.925 ; GJR 1.136 2.103
```

## See Also

- [`variance_forecast`](variance_forecast.md): the forecast
- [`ReturnsMetrics`](ReturnsMetrics.md): the result

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="pricing_variance_forecast.html">
    <span class="label">Previous</span>
    <span class="title">pricing_variance_forecast</span>
  </a>
  <a class="next-link" href="to_pricing_params.html">
    <span class="label">Next</span>
    <span class="title">to_pricing_params</span>
  </a>
</div>
<!-- apiref:nav:end -->
