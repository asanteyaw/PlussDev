<!-- generated from GLMLink.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `regression::GLMLink`

Selects the link function g that connects the mean μ of the response to the linear predictor η = Xβ, through g(μ) = η.

## Signature

```cpp
regression::GLMLink regression::GLMOptions::link = regression::GLMLink::Canonical;
regression::GLMLink regression::GLMFitResult::link;
```

`Canonical` is a request, not a link: `glm_fit` resolves it to the family's own link and records the link that was used on the result, so `fit.link` is never `Canonical`. The other three members name a link directly. The identity link has μ = η, the log link μ = e^η, and the logit link μ = 1 / (1 + e^(−η)).

## Fields

### `Canonical`

Uses the link that makes the family's sufficient statistic linear in the response: the identity for `Normal`, the logit for `Binomial` and the log for `Poisson`. It is the default, and the usual choice, because the likelihood is then concave and the iteration converges reliably. The program resolves it for each family and prints the link the result records.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto yn = pluss::make_series(std::vector<double>{1.1, 1.9, 3.2, 3.8, 5.1, 6.2, 6.8, 8.1, 9.0, 9.9}, {10}, as_f64());
  const auto yb = pluss::make_series(std::vector<double>{0, 0, 0, 1, 0, 1, 0, 1, 1, 1}, {10}, as_f64());
  const auto yp = pluss::make_series(std::vector<double>{1, 0, 2, 3, 2, 5, 4, 8, 9, 14}, {10}, as_f64());
  const auto name = [](reg::GLMLink link) { return link == reg::GLMLink::Identity ? "Identity" : link == reg::GLMLink::Log ? "Log" : link == reg::GLMLink::Logit ? "Logit" : "Canonical"; };
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Normal;
  out.writeln("Normal   resolves to", name(reg::glm_fit(yn, x, options).link));
  options.family = reg::GLMFamily::Binomial;
  out.writeln("Binomial resolves to", name(reg::glm_fit(yb, x, options).link));
  options.family = reg::GLMFamily::Poisson;
  out.writeln("Poisson  resolves to", name(reg::glm_fit(yp, x, options).link));
}
```

Output

```text
Normal   resolves to Identity
Binomial resolves to Logit
Poisson  resolves to Log
```

### `Identity`

The mean is the linear predictor itself, μ = η. For a `Normal` response this is the canonical link. For a `Binomial` response it models the probability as a straight line in the predictors, which is easy to read (each unit of x adds a fixed amount of probability) but unbounded: nothing stops the fitted probability from leaving [0, 1] away from the data. The program fits the pass/fail data both ways and prints the fitted probabilities.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{0, 0, 0, 1, 0, 1, 0, 1, 1, 1}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Binomial;
  options.link = reg::GLMLink::Identity;
  const auto identity = reg::glm_fit(y, x, options);
  options.link = reg::GLMLink::Logit;
  const auto logit = reg::glm_fit(y, x, options);
  out.writeln("identity coefficients", identity.coefficients.items<double>());
  out.writeln("identity fitted      ", identity.fitted_values.items<double>());
  out.writeln("logit    fitted      ", logit.fitted_values.items<double>());
  out.writeln("identity deviance", identity.deviance, " logit deviance", logit.deviance);
}
```

Output

```text
identity coefficients [-0.11111, 0.22222]
identity fitted       [-0.00000, 0.11111, 0.22222, 0.33333, 0.44444, 0.55556, 0.66667, 0.77778, 0.88889, 1.00000]
logit    fitted       [0.04543, 0.08561, 0.15554, 0.26599, 0.41621, 0.58379, 0.73401, 0.84446, 0.91439, 0.95457]
identity deviance 8.22199  logit deviance 8.67022
```

The identity line touches 0 at x = 0.5 and 1 at x = 5, the two ends of the data, so a prediction at x = 5.5 would already exceed one; the logit curve stays inside (0, 1) everywhere. The straight line has the lower deviance here, 8.22 against 8.67, which is a property of this sample and not a general advantage.

### `Log`

The mean is the exponential of the linear predictor, μ = e^η, so it is always positive and each unit of x multiplies the mean by a constant factor. It is canonical for `Poisson`, and it also suits a positive `Normal` response whose mean grows geometrically. With a `Binomial` response it gives probabilities that are positive but not capped at one. The program fits a `Normal` response with the log link and prints the multiplicative effect e^b₁ of one unit of x.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{1.1, 1.9, 3.2, 3.8, 5.1, 6.2, 6.8, 8.1, 9.0, 9.9}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Normal;
  options.link = reg::GLMLink::Log;
  const auto fit = reg::glm_fit(y, x, options);
  const auto b = fit.coefficients.items<double>();
  out.writeln("coefficients", b);
  out.writeln("one unit of x multiplies the mean by", std::exp(b[1]));
  out.writeln("fitted", fit.fitted_values.items<double>());
  out.writeln("iterations", fit.iterations, " converged", fit.converged);
}
```

Output

```text
coefficients [0.66662, 0.33984]
one unit of x multiplies the mean by 1.40472
fitted [2.30837, 2.73591, 3.24263, 3.84319, 4.55499, 5.39863, 6.39851, 7.58359, 8.98815, 10.65285]
iterations 16  converged true
```

### `Logit`

The log-odds of the mean are linear, g(μ) = ln(μ / (1 − μ)), so the fitted mean always lies in (0, 1). It is canonical for `Binomial`. It needs a response in [0, 1] whatever the family, and `glm_fit` rejects any other response with `the logit link expects y in [0,1]`. The program fits the pass/fail data and then asks for the logit link on a count.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto pass = pluss::make_series(std::vector<double>{0, 0, 0, 1, 0, 1, 0, 1, 1, 1}, {10}, as_f64());
  const auto count = pluss::make_series(std::vector<double>{1, 0, 2, 3, 2, 5, 4, 8, 9, 14}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Binomial;
  options.link = reg::GLMLink::Logit;
  const auto fit = reg::glm_fit(pass, x, options);
  out.writeln("coefficients", fit.coefficients.items<double>(), " odds ratio per unit of x", std::exp(fit.coefficients.items<double>()[1]));
  options.family = reg::GLMFamily::Poisson;
  try {
    reg::glm_fit(count, x, options);
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find("Triggered")));
  }
}
```

Output

```text
coefficients [-3.72188, 1.35341]  odds ratio per unit of x 3.87061
subsp::stats::regression::glm_fit: the logit link expects y in [0,1]
```

## Guidance

A link also decides whether the iteration can fail. The canonical links give a concave log-likelihood; a non-canonical link can leave it flat or push the mean out of its legal range during the search. The `Poisson` family with the identity link, which needs a non-negative mean, does not converge on the count data of the other pages: `converged` is false after `max_iter` iterations and `warnings` says so. Check `converged` after any fit with a non-canonical link.

## See Also

[`GLMFamily`](GLMFamily.md) fixes the variance function that the link is paired with, [`GLMOptions`](GLMOptions.md) carries the choice into `glm_fit`, and [`GLMFitResult`](GLMFitResult.md) records the resolved link.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="GLMFamily.html">
    <span class="label">Previous</span>
    <span class="title">GLMFamily</span>
  </a>
  <a class="next-link" href="GLMPredictionType.html">
    <span class="label">Next</span>
    <span class="title">GLMPredictionType</span>
  </a>
</div>
<!-- apiref:nav:end -->
