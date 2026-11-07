<!-- generated from families_and_links.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Families and links

[Back to Generalised Linear Models](../regression_02_generalised_linear_models.md)

```cpp
enum class reg::GLMFamily { Normal, Binomial, Poisson };
enum class reg::GLMLink   { Canonical, Identity, Log, Logit };
```

The *family* fixes the variance of the response as a function of its mean and the likelihood that is maximised. The *link* g connects the mean to the linear predictor, g(μ) = η.

| Family | Response | Variance | Likelihood |
|---|---|---|---|
| `Normal` | any real | constant | Gaussian |
| `Binomial` | in [0, 1] | μ (1 − μ) | Bernoulli (a fraction between 0 and 1 is accepted) |
| `Poisson` | ≥ 0 | μ | Poisson |

| Link | Mean from the linear predictor | Range of the mean |
|---|---|---|
| `Identity` | μ = η | all reals |
| `Log` | μ = exp(η) | positive |
| `Logit` | μ = 1 / (1 + e^−η) | strictly between 0 and 1 |
| `Canonical` | the natural link of the family: identity for `Normal`, logit for `Binomial`, log for `Poisson` | |

`Canonical` is the default. The link that was actually used is reported in `fit.link`, never as `Canonical`. Any link can be paired with any family, with one restriction that follows from the table: the logit link keeps the mean strictly between 0 and 1, so it needs a response in [0, 1] whatever the family, and a response outside that range is rejected before any iteration.

## Every pair on one data set

The program fits all twelve pairs. The binomial family is given the pass/fail response of the previous pages and the other two the counts on the same predictor. Each line shows the link that was used, whether IRLS converged, the iterations, the coefficients and the deviance.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto hours = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10, 1}, as_f64());
  const auto passed = pluss::make_series(std::vector<double>{0, 0, 0, 1, 0, 1, 0, 1, 1, 1}, {10}, as_f64());
  const auto counts = pluss::make_series(std::vector<double>{1, 0, 2, 1, 3, 2, 4, 3, 6, 5}, {10}, as_f64());

  const std::vector<std::pair<const char *, reg::GLMFamily>> families{{"Normal  ", reg::GLMFamily::Normal}, {"Binomial", reg::GLMFamily::Binomial}, {"Poisson ", reg::GLMFamily::Poisson}};
  const std::vector<std::pair<const char *, reg::GLMLink>> links{{"Canonical", reg::GLMLink::Canonical}, {"Identity ", reg::GLMLink::Identity}, {"Log      ", reg::GLMLink::Log}, {"Logit    ", reg::GLMLink::Logit}};
  auto name = [](reg::GLMLink link) { return link == reg::GLMLink::Identity ? "identity" : link == reg::GLMLink::Log ? "log" : link == reg::GLMLink::Logit ? "logit" : "canonical"; };

  for (const auto &[family_name, family] : families) {
    for (const auto &[link_name, link] : links) {
      reg::GLMOptions options;
      options.family = family;
      options.link = link;
      try {
        const auto fit = reg::glm_fit(family == reg::GLMFamily::Binomial ? passed : counts, hours, options);
        out.writeln(family_name, link_name, "-> used", name(fit.link), " converged", fit.converged, " iterations", fit.iterations, " coefficients", fit.coefficients.pull<double>(), " deviance", fit.deviance);
      } catch (const std::exception &e) {
        const std::string message = e.what();
        out.writeln(family_name, link_name, "->", message.substr(message.find("glm_fit: ") + 9, message.find('\n') - message.find("glm_fit: ") - 9));
      }
    }
  }
}
```

Output

```text
Normal   Canonical -> used identity  converged true  iterations 1  coefficients [-0.3333, 1.1030]  deviance 7.0061
Normal   Identity  -> used identity  converged true  iterations 1  coefficients [-0.3333, 1.1030]  deviance 7.0061
Normal   Log       -> used log  converged true  iterations 12  coefficients [-0.2698, 0.4041]  deviance 7.0208
Normal   Logit     -> the logit link expects y in [0,1]
Binomial Canonical -> used logit  converged true  iterations 6  coefficients [-3.7219, 1.3534]  deviance 8.6702
Binomial Identity  -> used identity  converged true  iterations 3  coefficients [-0.1111, 0.2222]  deviance 8.2220
Binomial Log       -> used log  converged true  iterations 13  coefficients [-1.9688, 0.3938]  deviance 9.0867
Binomial Logit     -> used logit  converged true  iterations 6  coefficients [-3.7219, 1.3534]  deviance 8.6702
Poisson  Canonical -> used log  converged true  iterations 6  coefficients [-0.4120, 0.4409]  deviance 4.1316
Poisson  Identity  -> used identity  converged true  iterations 21  coefficients [0.0347, 0.9692]  deviance 4.4042
Poisson  Log       -> used log  converged true  iterations 6  coefficients [-0.4120, 0.4409]  deviance 4.1316
Poisson  Logit     -> the logit link expects y in [0,1]
```

`Canonical` resolves as stated: identity for the normal family, logit for the binomial and log for the Poisson, and the pair is reported as `used`. For the binomial response the identity link fits a straight line to the pass rate (deviance 8.22, a little below the logit's 8.67 on these ten points), and the log link models a multiplicative effect on the probability (deviance 9.09). For the counts, the log link has deviance 4.13 against 4.40 for the identity link, and the normal family with a log link has 7.02 against 7.01 for the identity link. The logit link with the normal and Poisson families is refused, because the response here is a count.

## What the link does away from the data

The link decides what the model predicts outside the observed range. For the pass/fail data the three links give the same kind of curve between 0.5 and 5 hours, but they differ beyond it. The identity link models the probability as a straight line, which leaves the interval [0, 1]; the log link models it as an exponential, which can exceed 1; only the logit keeps every prediction a valid probability.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto hours = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10, 1}, as_f64());
  const auto passed = pluss::make_series(std::vector<double>{0, 0, 0, 1, 0, 1, 0, 1, 1, 1}, {10}, as_f64());
  const auto grid = pluss::make_series(std::vector<double>{-2, 0, 1, 3, 5, 7, 10}, {7, 1}, as_f64());

  out.writeln("hours          ", grid.reshape({7}).pull<double>());
  const std::vector<std::pair<const char *, reg::GLMLink>> links{{"identity link", reg::GLMLink::Identity}, {"log link     ", reg::GLMLink::Log}, {"logit link   ", reg::GLMLink::Logit}};
  for (const auto &[label, link] : links) {
    reg::GLMOptions options;
    options.family = reg::GLMFamily::Binomial;
    options.link = link;
    const auto fit = reg::glm_fit(passed, hours, options);
    out.writeln(label, " P(pass)", reg::glm_predict(fit, grid).pull<double>(), " linear predictor", reg::glm_predict(fit, grid, reg::GLMPredictionType::Linear).pull<double>());
  }
}
```

Output

```text
hours           [-2.0000, 0.0000, 1.0000, 3.0000, 5.0000, 7.0000, 10.0000]
identity link  P(pass) [-0.5556, -0.1111, 0.1111, 0.5556, 1.0000, 1.4444, 2.1111]  linear predictor [-0.5556, -0.1111, 0.1111, 0.5556, 1.0000, 1.4444, 2.1111]
log link       P(pass) [0.0635, 0.1396, 0.2070, 0.4550, 1.0000, 2.1979, 7.1618]  linear predictor [-2.7563, -1.9688, -1.5750, -0.7875, 0.0000, 0.7875, 1.9688]
logit link     P(pass) [0.0016, 0.0236, 0.0856, 0.5838, 0.9546, 0.9968, 0.9999]  linear predictor [-6.4287, -3.7219, -2.3685, 0.3384, 3.0452, 5.7520, 9.8122]
```

Each model has its own linear predictor η, and its link turns η into a mean. With the identity link the 'probability' is −0.56 at −2 hours and 2.11 at 10 hours. With the log link it is exp(η), reaching 7.16 at 10 hours. Only the logit keeps every prediction between 0 and 1, from 0.0016 to 0.9999. Within the observed range the three curves are close; the choice of link matters most where the data are scarce.

## The logit link needs a response in [0, 1]

The logit link can only produce means between 0 and 1, so a count of 6 could never be matched, and fitting it would drive the coefficients toward infinity without settling. `glm_fit` therefore refuses a logit link whenever the response leaves [0, 1], for any family, with the message `the logit link expects y in [0,1]`. The same link is accepted, with any family, for a response that does lie in that range: a normal-family fit of proportions with a logit link models their mean on the log-odds scale.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto hours = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10, 1}, as_f64());
  const auto counts = pluss::make_series(std::vector<double>{1, 0, 2, 1, 3, 2, 4, 3, 6, 5}, {10}, as_f64());
  const auto share = pluss::make_series(std::vector<double>{0.05, 0.08, 0.12, 0.20, 0.31, 0.42, 0.55, 0.66, 0.78, 0.86}, {10}, as_f64());

  for (auto family : {reg::GLMFamily::Normal, reg::GLMFamily::Poisson}) {
    reg::GLMOptions options;
    options.family = family;
    options.link = reg::GLMLink::Logit;
    try {
      reg::glm_fit(counts, hours, options);
    } catch (const std::exception &e) {
      const std::string message = e.what();
      out.writeln(family == reg::GLMFamily::Normal ? "Normal  family, logit link, counts   ->" : "Poisson family, logit link, counts   ->", message.substr(0, message.find('\n')));
    }
  }

  reg::GLMOptions options;
  options.family = reg::GLMFamily::Normal;
  options.link = reg::GLMLink::Logit;
  const auto fit = reg::glm_fit(share, hours, options);
  out.writeln("Normal family, logit link, proportions -> coefficients", fit.coefficients.pull<double>(), " converged", fit.converged, " iterations", fit.iterations);
  out.writeln("fitted proportions", fit.fitted_values.pull<double>());
}
```

Output

```text
Normal  family, logit link, counts   -> subsp::stats::regression::glm_fit: the logit link expects y in [0,1]
Poisson family, logit link, counts   -> subsp::stats::regression::glm_fit: the logit link expects y in [0,1]
Normal family, logit link, proportions -> coefficients [-3.4812, 1.0508]  converged true  iterations 7
fitted proportions [0.0495, 0.0809, 0.1295, 0.2011, 0.2986, 0.4186, 0.5490, 0.6731, 0.7769, 0.8548]
```

Both counts are refused with the message shown, whichever family is asked for. The normal family with the logit link is accepted for proportions, converges in 7 iterations, and returns fitted proportions between 0.05 and 0.85 that follow the data.
