<!-- generated from dose_response_and_the_median_effective_dose.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Dose-response and the median effective dose

[Back to Worked Problems](../regression_05_worked_problems.md)

In a dose-response study, subjects receive a dose and either respond or do not. The probability of a response rises with the dose along a logistic curve, and the quantity of interest is the dose at which half of the subjects respond: the *median effective dose*, ED50. For the logistic model logit p = b₀ + b₁d, the probability is one half where the linear predictor is zero, so ED50 = −b₀ / b₁. It is a ratio of two estimated coefficients, so its uncertainty needs their covariance.

The study has eight doses (1 to 8), ten subjects each. The responses are generated from a true curve with ED50 = 3 and slope 1.2 by comparing a fixed low-discrepancy sequence with the true probability.

## The estimate and a delta-method interval

The delta method gives Var(ED50) ≈ [Var(b₀) + 2 ED50 Cov(b₀, b₁) + ED50² Var(b₁)] / b₁², obtained from the gradient (−1 / b₁, b₀ / b₁²) of the ratio and the covariance matrix of the coefficients.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  std::vector<double> dose, responded;
  for (int level = 1; level <= 8; ++level) {
    for (int subject = 0; subject < 10; ++subject) {
      const int i = (level - 1) * 10 + subject;
      const double p = 1.0 / (1.0 + std::exp(-1.2 * (level - 3.0)));
      const double u = std::fmod(i * 0.6180339887498949, 1.0);  // a fixed low-discrepancy sequence in place of random draws
      dose.push_back(level);
      responded.push_back(u < p ? 1.0 : 0.0);
    }
  }
  const auto x = pluss::make_series(dose, {80}, as_f64());
  const auto y = pluss::make_series(responded, {80}, as_f64());

  reg::GLMOptions options;
  options.family = reg::GLMFamily::Binomial;
  options.response_name = "responded";
  options.predictor_names = {"dose"};
  const auto fit = reg::glm_fit(y, x, options);
  const auto b = fit.coefficients.pull<double>();
  const auto cov = fit.covariance.host<double>().array().data;

  out.writeln("coefficients", b, " std errors", fit.std_errors.pull<double>(), " converged", fit.converged);
  out.writeln("responses by dose level:");
  for (int level = 1; level <= 8; ++level) {
    double count = 0.0;
    for (int subject = 0; subject < 10; ++subject) {
      count += responded[(level - 1) * 10 + subject];
    }
    out.writeln("   dose", level, "responded", count, "of 10   fitted probability", reg::glm_predict(fit, pluss::make_series(std::vector<double>{static_cast<double>(level)}, {1}, as_f64())).pull<double>()[0]);
  }

  const double ed50 = -b[0] / b[1];
  const double variance = (cov[0] + 2.0 * ed50 * cov[1] + ed50 * ed50 * cov[3]) / (b[1] * b[1]);
  out.writeln("ED50 =", ed50, " std error", std::sqrt(variance), " interval [", ed50 - 1.96 * std::sqrt(variance), ",", ed50 + 1.96 * std::sqrt(variance), "]   (true value 3)");
  out.writeln("the model's probability at the ED50 is", reg::glm_predict(fit, pluss::make_series(std::vector<double>{ed50}, {1}, as_f64())).pull<double>()[0]);
}
```

Output

```text
coefficients [-2.9968, 1.0235]  std errors [0.8034, 0.2293]  converged true
responses by dose level:
   dose 1 responded 1.0000 of 10   fitted probability 0.1220
   dose 2 responded 3.0000 of 10   fitted probability 0.2789
   dose 3 responded 5.0000 of 10   fitted probability 0.5184
   dose 4 responded 8.0000 of 10   fitted probability 0.7498
   dose 5 responded 9.0000 of 10   fitted probability 0.8929
   dose 6 responded 9.0000 of 10   fitted probability 0.9587
   dose 7 responded 10.0000 of 10   fitted probability 0.9848
   dose 8 responded 10.0000 of 10   fitted probability 0.9945
ED50 = 2.9279  std error 0.3306  interval [ 2.2799 , 3.5759 ]   (true value 3)
the model's probability at the ED50 is 0.5000
```

The fit converges with coefficients of −2.9968 (standard error 0.80) and 1.0235 (0.23). The counts rise from 1 response in 10 at dose 1 to 10 in 10 at dose 7, and the fitted probability passes one half between doses 2 and 3 (0.5184 at dose 3). The ED50 is estimated as 2.928 against the true value 3, with a standard error of 0.33 and a delta-method interval of [2.28, 3.58] that contains it. The model's probability at the estimated ED50 is exactly 0.5, as the definition requires.

## An interval that respects the ratio

A symmetric interval is a poor description of a ratio whose denominator is uncertain. *Fieller's* approach avoids the approximation: a trial dose m is plausible if the intercept of the model re-centred at m, b₀ + b₁m, is not significantly different from zero. That intercept has the variance Var(b₀) + 2m Cov(b₀, b₁) + m² Var(b₁), and the set of plausible m is found by scanning. When the slope is estimated precisely the Fieller and delta-method intervals nearly coincide; when it is not, the Fieller interval becomes asymmetric or unbounded, which is the honest answer.

The program scans m and reports the Fieller interval for the full study and for a thin version of it with two subjects per dose, where the slope is less certain.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  auto study = [&](int per_dose) {
    std::vector<double> dose, responded;
    for (int level = 1; level <= 8; ++level) {
      for (int subject = 0; subject < per_dose; ++subject) {
        const int i = (level - 1) * 10 + subject;
        const double p = 1.0 / (1.0 + std::exp(-1.2 * (level - 3.0)));
        dose.push_back(level);
        responded.push_back(std::fmod(i * 0.6180339887498949, 1.0) < p ? 1.0 : 0.0);
      }
    }
    const auto n = static_cast<std::int64_t>(dose.size());
    reg::GLMOptions options;
    options.family = reg::GLMFamily::Binomial;
    return reg::glm_fit(pluss::make_series(responded, {n}, as_f64()), pluss::make_series(dose, {n}, as_f64()), options);
  };

  for (int per_dose : {10, 2}) {
    const auto fit = study(per_dose);
    const auto b = fit.coefficients.pull<double>();
    const auto cov = fit.covariance.host<double>().array().data;
    const double ed50 = -b[0] / b[1];
    const double variance = (cov[0] + 2.0 * ed50 * cov[1] + ed50 * ed50 * cov[3]) / (b[1] * b[1]);

    // Plausible doses: those at which the re-centred intercept is within 1.96 standard errors of zero.
    double lowest = 1e300, highest = -1e300;
    for (double m = -20.0; m <= 30.0; m += 0.001) {
      const double centred = b[0] + b[1] * m;
      const double var = cov[0] + 2.0 * m * cov[1] + m * m * cov[3];
      if (centred * centred <= 1.96 * 1.96 * var) {
        lowest = std::min(lowest, m);
        highest = std::max(highest, m);
      }
    }
    out.writeln(per_dose, "subjects per dose: ED50", ed50, " slope z", fit.z_values.pull<double>()[1]);
    out.writeln("   delta method  [", ed50 - 1.96 * std::sqrt(variance), ",", ed50 + 1.96 * std::sqrt(variance), "]   Fieller  [", lowest, ",", highest, "]");
  }
}
```

Output

```text
10 subjects per dose: ED50 2.9279  slope z 4.4639
   delta method  [ 2.2799 , 3.5759 ]   Fieller  [ 2.1420 , 3.5890 ]
2 subjects per dose: ED50 1.7291  slope z 1.6367
   delta method  [ -0.1079 , 3.5661 ]   Fieller  [ -20.0000 , 29.9990 ]
```

With ten subjects per dose the Fieller interval, [2.142, 3.589], is close to the delta-method one, [2.280, 3.576], but wider on the low side: it extends 0.79 below the estimate and 0.66 above it, an asymmetry that the symmetric interval cannot show. With two subjects per dose the slope is no longer clearly different from zero (z = 1.64), and the two methods part company. The delta method still reports a finite interval, [−0.11, 3.57], which even includes negative doses. The Fieller scan finds every dose in the range searched (−20 to 30) plausible, because if the slope might be zero then the curve may be flat at one half and no dose is excluded. The honest conclusion is that this smaller study does not locate the ED50.
