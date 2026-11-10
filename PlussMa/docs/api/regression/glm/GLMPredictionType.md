<!-- generated from GLMPredictionType.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `regression::GLMPredictionType`

Chooses the scale on which `glm_predict` returns its answer: the mean of the response, or the linear predictor before the inverse link.

## Signature

```cpp
Series regression::glm_predict(const regression::GLMFitResult &fit, const Series &x, regression::GLMPredictionType type = regression::GLMPredictionType::Mean, Config cfg = {});
Series regression::glm_predict(const regression::GLMFitResult &fit, const Series &x, const Series &offset, regression::GLMPredictionType type = regression::GLMPredictionType::Mean, Config cfg = {});
```

The two values are the two ends of the link: with link g, `Linear` returns η = g(μ) and `Mean` returns μ = g⁻¹(η). They agree exactly when the link is the identity.

## Fields

### `Mean`

The default. The linear predictor is passed through the inverse link, so the answer is on the scale of the response: an expected count for `Poisson`, a probability for `Binomial`, an expected value for `Normal`. This is the number to report, plot against the data or feed into a decision. The program predicts the expected count at three new doses.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{1, 0, 2, 3, 2, 5, 4, 8, 9, 14}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Poisson;
  const auto fit = reg::glm_fit(y, x, options);
  const auto doses = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}, {3}, as_f64());
  out.writeln("expected counts", reg::glm_predict(fit, doses, reg::GLMPredictionType::Mean).items<double>());
  out.writeln("default is the same", reg::glm_predict(fit, doses).items<double>());
}
```

Output

```text
expected counts [1.08664, 2.04267, 3.83982]
default is the same [1.08664, 2.04267, 3.83982]
```

### `Linear`

The linear predictor η = Xβ itself, on the scale of the link: log-counts for a Poisson fit, log-odds for a logistic fit. Use it to compare predictors, to build intervals on the link scale before transforming them back, and to check the model without the nonlinearity of the inverse link. The program prints the linear predictor of the same three doses and confirms that the exponential of it is the mean.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{1, 0, 2, 3, 2, 5, 4, 8, 9, 14}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Poisson;
  const auto fit = reg::glm_fit(y, x, options);
  const auto doses = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}, {3}, as_f64());
  const auto eta = reg::glm_predict(fit, doses, reg::GLMPredictionType::Linear);
  out.writeln("linear predictor", eta.items<double>());
  out.writeln("exp(linear)     ", eta.exp().items<double>());
  out.writeln("mean            ", reg::glm_predict(fit, doses).items<double>());
}
```

Output

```text
linear predictor [0.08309, 0.71426, 1.34543]
exp(linear)      [1.08664, 2.04267, 3.83982]
mean             [1.08664, 2.04267, 3.83982]
```

### Mean and linear for a logistic fit

For a `Binomial` fit the two scales are the probability and the log-odds, and the logit links them: the log-odds of the mean equal the linear predictor.

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
  const auto fit = reg::glm_fit(y, x, options);
  const auto points = pluss::make_series(std::vector<double>{1.0, 3.0, 5.0}, {3}, as_f64());
  const auto p = reg::glm_predict(fit, points);
  const auto eta = reg::glm_predict(fit, points, reg::GLMPredictionType::Linear);
  out.writeln("probability", p.items<double>());
  out.writeln("log-odds   ", eta.items<double>());
  out.writeln("log(p / (1 - p))", p.div(p.neg().add(1.0)).log().items<double>());
}
```

Output

```text
probability [0.08561, 0.58379, 0.95457]
log-odds    [-2.36847, 0.33835, 3.04518]
log(p / (1 - p)) [-2.36847, 0.33835, 3.04518]
```

## Guidance

Transforming an average of linear predictors does not give the average of the means, so average on the `Mean` scale when the quantity wanted is an expected value. When a fit was made with an `offset`, pass the offset of the new rows to the overload that takes one; the offset is added to the linear predictor before the inverse link and the scale choice applies after it.

## See Also

[`glm_predict`](glm_predict.md) takes the choice as its last parameter, [`GLMLink`](GLMLink.md) defines the inverse link that `Mean` applies, and [`GLMFitResult`](GLMFitResult.md) carries the coefficients being applied.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="GLMLink.html">
    <span class="label">Previous</span>
    <span class="title">GLMLink</span>
  </a>
  <a class="next-link" href="GLMOptions.html">
    <span class="label">Next</span>
    <span class="title">GLMOptions</span>
  </a>
</div>
<!-- apiref:nav:end -->
