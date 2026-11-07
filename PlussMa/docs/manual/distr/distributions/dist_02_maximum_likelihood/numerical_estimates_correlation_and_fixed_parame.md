<!-- generated from numerical_estimates_correlation_and_fixed_parame.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Numerical estimates, correlation and fixed parameters

[Back to Maximum Likelihood](../dist_02_maximum_likelihood.md)

For the gamma, the beta and the negative binomial the likelihood equations have no closed-form solution. They are solved numerically to full precision: Newton's method for the gamma and beta, a bracketed root search for the negative binomial. The result is the exact maximiser, in the same sense as the closed-form fits; what changes is the shape of the answer. With two parameters estimated *jointly*, the estimates are no longer independent, and the covariance between them is as important as the standard errors. And some parameters are not estimated at all: a gamma has a location, a beta a support, a binomial a number of trials, and these are *held fixed* at the values stored in the object you call `MLE` on.

Two conventions run through this page.

- **The fit object is the starting point and the specification.** `distr::Gamma{1.0, 1.0, 0.0}.MLE(x)` estimates the shape and scale and takes the location, 0, from the object. The shape and scale in the object are only starting values.
- **Fixed parameters have zero uncertainty.** They appear in `vcov` as a zero row and column, in `stderr` as 0, and in `confint` as an interval of zero width, so a covariance matrix always has one row per parameter of the family.

The programs use `namespace distr = pluss::subsp::stats::distr;` and `using pluss::Series;`.

## Correlated estimates: the gamma

The gamma has a mean `shape × scale` that data determine much more precisely than either parameter on its own: a larger shape goes with a smaller scale. The program fits a gamma (true shape 2.5, scale 1.3, location 0) to 500 observations, and prints the estimates, the standard errors, the correlation of shape and scale from the covariance matrix, and the standard error of their product by the delta method.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

#include <vector>

namespace distr = pluss::subsp::stats::distr;
namespace dst = distr::dst;
using namespace pluss::tils;

int main() {
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));

  const auto x = distr::Gamma{2.5, 1.3, 0.0}.sample({500}, f64, 20261022ULL);
  const auto fit = distr::Gamma{1.0, 1.0, 0.0}.MLE(x);
  out.writeln("shape", fit.shape, "  scale", fit.scale, "  location (fixed)", fit.location, "   (true 2.5, 1.3, 0)");
  out.writeln("stderr        ", fit.stderr(x).host<double>());
  out.writeln("95% intervals ", fit.confint(x).host<double>());

  const auto v = fit.vcov(x).host<double>();  // 3 x 3, in the order shape, scale, location
  out.writeln("\ncorrelation of the shape and scale estimates:", v[1] / std::sqrt(v[0] * v[4]));

  // The mean shape * scale: variance by the delta method, d/dshape = scale, d/dscale = shape.
  const double var_mean = fit.scale * fit.scale * v[0] + fit.shape * fit.shape * v[4] + 2 * fit.shape * fit.scale * v[1];
  out.writeln("estimated mean", fit.shape * fit.scale, "  standard error", std::sqrt(var_mean), "  relative", std::sqrt(var_mean) / (fit.shape * fit.scale));
  out.writeln("relative standard errors: shape", std::sqrt(v[0]) / fit.shape, ", scale", std::sqrt(v[4]) / fit.scale);
  out.writeln("log-likelihood", fit.loglik(x).item<double>(), PrintOptions{}.fixed(2));
}
```

Output

```text
shape 2.4422   scale 1.3758   location (fixed) 0.0000    (true 2.5, 1.3, 0)
stderr         [0.1452, 0.0908, 0.0000]
95% intervals  [2.1577, 2.7267, 1.1979, 1.5536, 0.0000, 0.0000]

correlation of the shape and scale estimates: -0.9010
estimated mean 3.3600   standard error 0.0962   relative 0.0286
relative standard errors: shape 0.0594 , scale 0.0660
log-likelihood -1016.73
```

The estimates (2.44 and 1.38 for a true 2.5 and 1.3) are within one standard error of the truth, with relative standard errors of about 6% each, and their correlation is strongly negative, −0.90: data that look slightly more skewed than average push the shape down and the scale up together. The mean `shape × scale` is estimated as 3.36 with a standard error of 0.096, or 2.9% relative, about half the relative uncertainty of either parameter. The delta-method standard error uses the covariance, and the strong negative correlation makes the two errors partly cancel in the product. This is why the covariance matters: **the standard errors of the individual parameters alone would overstate the uncertainty of any combination of them**, and a function of the parameters (a mean, a quantile, a tail probability) should be computed with `vcov`, not with `stderr`.

## Fixed parameters are part of the model

A held-fixed parameter is a modelling decision, and getting it wrong is not caught by the fit: it just estimates the others as well as it can *given* the wrong value. The program draws data from a gamma with a location of 2 (the data start at 2, not 0) and fits it three ways: with the location left at 0 (wrong), with the right location of 2, and with a nearby wrong value of 1.5.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

#include <vector>

namespace distr = pluss::subsp::stats::distr;
namespace dst = distr::dst;
using namespace pluss::tils;

int main() {
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  const auto x = distr::Gamma{2.5, 1.3, 2.0}.sample({500}, f64, 41ULL);  // shape 2.5, scale 1.3, location 2

  out.writeln("location held at   shape   scale   log-likelihood");
  for (double loc : {0.0, 1.5, 2.0}) {
    const auto fit = distr::Gamma{1.0, 1.0, loc}.MLE(x);
    out.writeln(loc, "              ", fit.shape, fit.scale, fit.loglik(x).item<double>());
  }
  out.writeln("(true: location 2, shape 2.5, scale 1.3)");
}
```

Output

```text
location held at   shape   scale   log-likelihood
0.000                8.129 0.643 -991.578
1.500                3.941 0.946 -979.606
2.000                2.751 1.174 -976.257
(true: location 2, shape 2.5, scale 1.3)
```

With the location at 0 the model is wrong, and it compensates: the shape is estimated at 8.1 (the truth is 2.5) and the scale at 0.64 (truth 1.3), because the only way a distribution that starts at 0 can describe data that start at 2 is to be narrow and nearly bell-shaped. The log-likelihood is 15 points lower than with the right location (−991.6 against −976.3). With the right location both parameters are recovered (2.75 and 1.17, within sampling error of 2.5 and 1.3). A nearby wrong value (1.5) is in between (shape 3.9, log-likelihood −979.6). The lesson is general: **fix only what you know**; if the location is unknown, look at the data (the minimum is an upper bound for it) or fit a family whose support is not tied to a number you had to guess. The log-likelihood is the diagnostic: if you can find a fixed value that raises it substantially, the first value was wrong.

## The beta's support and the negative binomial

The beta distribution is fitted on the support stored in the object (`Beta{low, high, alpha, beta}`), and only `alpha` and `beta` are estimated. The same data moved to a different interval and fitted with a beta on that interval give the same parameters, which is a good check that you have told the fit the right support. The negative binomial's two parameters `r` and `p` are strongly correlated too. The program fits both, and also shows what happens with *under-dispersed* counts, which a negative binomial cannot describe.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

#include <vector>

namespace distr = pluss::subsp::stats::distr;
namespace dst = distr::dst;
using namespace pluss::tils;

int main() {
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));

  const auto xb = distr::Beta{0.0, 1.0, 2.5, 4.0}.sample({500}, f64, 7ULL);
  const auto bf = distr::Beta{0.0, 1.0, 1.0, 1.0}.MLE(xb);
  out.writeln("Beta sample on [0, 1] (true alpha 2.5, beta 4):  alpha", bf.alpha, " beta", bf.beta, " (support", bf.low, "to", bf.high, "is fixed)");
  out.writeln("  stderr  ", bf.stderr(xb).host<double>());
  const auto moved = xb * 4.0 - 1.0;  // the same data on [-1, 3]
  const auto bf2 = distr::Beta{-1.0, 3.0, 1.0, 1.0}.MLE(moved);
  out.writeln("  the same data on [-1, 3], told so:  alpha", bf2.alpha, " beta", bf2.beta);

  const auto xn = distr::NegativeBinomial{3.5, 0.4}.sample({500}, f64, 11ULL);
  const auto nf = distr::NegativeBinomial{1.0, 0.5}.MLE(xn);
  const auto nv = nf.vcov(xn).host<double>();
  out.writeln("\nNegative binomial sample (true r 3.5, p 0.4):  r", nf.r, " p", nf.p);
  out.writeln("  stderr        ", nf.stderr(xn).host<double>());
  out.writeln("  95% intervals ", nf.confint(xn).host<double>());
  out.writeln("  correlation of r and p:", nv[1] / std::sqrt(nv[0] * nv[3]));

  // Counts whose variance is below their mean have no finite maximum: r grows without bound (the Poisson limit).
  const auto flat = distr::NegativeBinomial{1.0, 0.5}.MLE(distr::Binomial{10, 0.5}.sample({400}, f64, 13ULL));
  out.writeln("\nunder-dispersed counts (a binomial sample): r =", flat.r, PrintOptions{}.sci(1));
}
```

Output

```text
Beta sample on [0, 1] (true alpha 2.5, beta 4):  alpha 2.4846  beta 4.0644  (support 0.0000 to 1.0000 is fixed)
  stderr   [0.0000, 0.0000, 0.1490, 0.2534]
  the same data on [-1, 3], told so:  alpha 2.4846  beta 4.0644

Negative binomial sample (true r 3.5, p 0.4):  r 3.6458  p 0.4093
  stderr         [0.4118, 0.0283]
  95% intervals  [2.8388, 4.4529, 0.3539, 0.4647]
  correlation of r and p: 0.9655

under-dispersed counts (a binomial sample): r = 1.0e+08
```

The beta recovers its parameters (2.48 and 4.06 for a true 2.5 and 4, well within the standard errors of 0.15 and 0.25) and gives identical shape parameters on the stretched support, as it should; the first two entries of the `stderr` row are the fixed `low` and `high`, with zero uncertainty. For the negative binomial, `r` and `p` are almost perfectly correlated (0.97), so the interval for either alone is wide (`r` from 2.84 to 4.45 for a true 3.5) while the combination that fixes the mean is tight. The last line is the boundary case: a binomial sample has *less* spread than a Poisson with the same mean, and the negative binomial can only add spread. The likelihood therefore increases without limit as `r` grows (the family tends to the Poisson), and the fit stops at the largest value it searches, `r = 10⁸`. When you see an enormous `r`, the data are not over-dispersed, and a Poisson (or a binomial) is the right model.

## How much does the shape estimate vary?

The gamma shape is the harder of the two parameters to pin down. The program repeats the fit on 200 samples of 40 observations and 200 of 400, and reports the mean, spread and interval coverage of the shape estimate (true value 2.5).

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

#include <vector>

namespace distr = pluss::subsp::stats::distr;
namespace dst = distr::dst;
using namespace pluss::tils;

int main() {
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  out.writeln("    n   mean estimate   std dev   mean stderr   coverage");
  for (int n : {40, 400}) {
    const int reps = 200;
    double sum = 0.0, sum_sq = 0.0, se_sum = 0.0;
    int covered = 0;
    for (int r = 0; r < reps; ++r) {
      const auto x = distr::Gamma{2.5, 1.3, 0.0}.sample({n}, f64, 5000 + r);
      const auto fit = distr::Gamma{1.0, 1.0, 0.0}.MLE(x);
      sum += fit.shape;
      sum_sq += fit.shape * fit.shape;
      se_sum += fit.stderr(x).host<double>()[0];
      const auto ci = fit.confint(x).host<double>();  // rows: shape, scale, location
      covered += ci[0] <= 2.5 && 2.5 <= ci[1];
    }
    const double mean = sum / reps;
    out.writeln(n, "  ", mean, "  ", std::sqrt((sum_sq - reps * mean * mean) / (reps - 1)), "  ", se_sum / reps, "  ", static_cast<double>(covered) / reps);
  }
}
```

Output

```text
    n   mean estimate   std dev   mean stderr   coverage
40    2.639    0.573    0.557    0.960
400    2.533    0.180    0.169    0.945
```

Three things to read. The mean estimate sits above the true 2.5 at 40 observations (2.64) and close to it at 400 (2.53): the shape estimate is biased upward in small samples, and the bias fades with `n`. The spread across samples (the standard deviation of the estimates, 0.573 and 0.180) is what the reported standard errors try to predict, and the average *reported* standard error (0.557 and 0.169) is close to it, so the numbers the fit reports are the right size (a little too small, as Wald standard errors tend to be). And the interval coverage is 0.96 and 0.945, within noise of 0.95 (each is based on 200 samples, so has a standard error of about 0.015): the intervals can be trusted here even at 40 observations, as long as you remember that the estimate itself is a little high.

## Decisions and pitfalls

- **Set the fixed parameters deliberately.** They come from the object you call `MLE` on, not from the data; a wrong location, support or trial count gives a confidently wrong fit.
- **Use `vcov`, not `stderr`, for combinations.** Estimates are correlated; the uncertainty of a mean, quantile or probability depends on the covariance.
- **Expect small-sample bias.** The shape of a gamma and the dispersion of a negative binomial are biased in small samples; treat the interval as approximate.
- **An enormous `r` means no over-dispersion.** The negative binomial returns its upper limit for under- or equi-dispersed counts; use the Poisson or the binomial.
- **The log-likelihood is the test of a fixed value.** If changing a fixed parameter raises it a lot, the first choice was wrong.

Where next: [Chapter 3](../dist_03_heavy_tails_and_mixtures.md) fits heavy-tailed and multimodal distributions, which are only available numerically.
