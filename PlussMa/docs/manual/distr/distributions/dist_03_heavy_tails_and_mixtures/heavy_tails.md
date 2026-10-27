<!-- generated from heavy_tails.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Heavy tails

[Back to Heavy Tails and Mixtures](../dist_03_heavy_tails_and_mixtures.md)

The normal distribution is the default model for a bell-shaped variable, and it is often wrong in a specific way: it puts far too little probability on extreme values. Financial returns, insurance claims, river flows and measurement errors all produce observations several standard deviations from the mean much more often than a normal predicts, and they are often skewed as well. A normal fitted to such data matches the centre and fails exactly where the risk lies.

The **normal inverse Gaussian** (NIG) is the standard remedy: a four-parameter family with a parameter for the tail weight and one for the skewness. Its density is

> `f(x) = α δ K₁(α q) / (π q) · exp(δ γ + β (x − μ))`,  `q = √(δ² + (x − μ)²)`,  `γ = √(α² − β²)`,

with `K₁` the modified Bessel function of the second kind. The four parameters are `alpha` (tail weight: the tails decay like `exp(−(α − |β|)|x|)`, so a **smaller alpha means heavier tails**), `beta` (skewness; negative gives a longer left tail, and `|β| < α`), `location` and `scale`. The NIG with large `alpha` is nearly normal. `Nig::MLE(x)` maximises the exact log-likelihood over all four parameters from several starting shapes and keeps the best. The generalised hyperbolic family `Gh` adds an order `λ` of the Bessel function: `λ = −1/2` is exactly the NIG, `λ = 1` the hyperbolic distribution.

The programs use `namespace distr = pluss::subsp::stats::distr;` and fit samples drawn from known distributions.

## Fitting and comparing with the normal

The first program draws 2000 observations from a skewed NIG (`alpha 1.5`, `beta −0.4`, `location 0.1`, `scale 0.8`), fits both a normal and a NIG, and compares them by log-likelihood and Akaike's criterion, and by what they say about the left tail: the probability of a value below −3, which is about three standard deviations out.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

#include <vector>

namespace distr = pluss::subsp::stats::distr;
namespace dst = distr::dst;
using namespace pluss::tils;

int main() {
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto x = distr::Nig{1.5, -0.4, 0.1, 0.8}.sample({2000}, f64, 20261022ULL);

  const auto normal = distr::Normal{}.MLE(x);
  const auto nig = distr::Nig{}.MLE(x);
  out.writeln("true NIG(1.5, -0.4, 0.1, 0.8)");
  out.writeln("fitted NIG   alpha", nig.alpha, " beta", nig.beta, " location", nig.location, " scale", nig.scale);
  out.writeln("std errors  ", nig.stderr(x).host<double>());

  const double ll_n = normal.loglik(x).item<double>(), ll_g = nig.loglik(x).item<double>();
  out.writeln("\nmodel      loglik      AIC");
  out.writeln("normal  ", ll_n, 2 * 2 - 2 * ll_n, PrintOptions{}.fixed(2));
  out.writeln("NIG     ", ll_g, 2 * 4 - 2 * ll_g, PrintOptions{}.fixed(2));

  // Probability of a fall below a cut-off: the sample, the normal fit and the NIG fit.
  out.writeln("\ncut-off   sample   normal fit   NIG fit");
  for (double cut : {-2.0, -3.0, -4.0}) {
    const double below = (x < cut).to(pluss::ScalarType::Float64).mean().item<double>();
    const auto t = pluss::make_series({cut}, f64);
    out.writeln(cut, "  ", below, "  ", normal.model().cdf(t).item<double>(), "  ", nig.model().cdf(t).item<double>(), PrintOptions{}.fixed(5));
  }
}
```

Output

```text
true NIG(1.5, -0.4, 0.1, 0.8)
fitted NIG   alpha 1.7454  beta -0.5035  location 0.1308  scale 0.8595
std errors   [0.1829, 0.0998, 0.0421, 0.0677]

model      loglik      AIC
normal   -2261.36 4526.73
NIG      -2160.58 4329.16

cut-off   sample   normal fit   NIG fit
-2.00000    0.01600    0.00626    0.01869
-3.00000    0.00350    0.00006    0.00364
-4.00000    0.00150    0.00000    0.00076
```

The NIG fit recovers the generating parameters to within about one standard error (`alpha` 1.75 against 1.5 with a standard error of 0.18; `beta` −0.50 against −0.4 with 0.10); the tail parameter is the least precisely estimated. The log-likelihood gain over the normal is about 100 units for two extra parameters, so Akaike's criterion falls from 4527 to 4329: the evidence for heavy tails is overwhelming. The tail probabilities show *why it matters*. Beyond −2 the sample has 1.6% of its values, the NIG fit 1.9% and the normal fit 0.6%, a factor of 2.6 too small. At −3 the sample has 0.35%, the NIG 0.36% and the normal 0.006%: sixty times too small. At −4 only three of the 2000 observations fall there (0.15%); the NIG gives 0.08% and the normal essentially zero. A value-at-risk or loss probability computed from the normal would be badly optimistic; the NIG gets it right because it was allowed to put weight in the tail.

## When the data are not heavy-tailed

A flexible family should not invent heavy tails. If the data really are normal, the NIG should fit about as well as the normal with *large* `alpha` (the tail parameter running to the near-normal limit) and offer no real gain. This is also the test of whether you need the extra parameters: the program fits both to normal data, and compares log-likelihoods and criteria.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

#include <vector>

namespace distr = pluss::subsp::stats::distr;
namespace dst = distr::dst;
using namespace pluss::tils;

int main() {
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  out.writeln("    n  NIG alpha  loglik gain  AIC normal  AIC NIG");
  for (int n : {200, 2000}) {
    const auto x = distr::Normal{0.0, 1.0}.sample({n}, f64, 31ULL);
    const auto normal = distr::Normal{}.MLE(x);
    const auto nig = distr::Nig{}.MLE(x);
    const double ln = normal.loglik(x).item<double>(), lg = nig.loglik(x).item<double>();
    out.writeln(n, "  ", nig.alpha, "  ", lg - ln, "  ", 4 - 2 * ln, "  ", 8 - 2 * lg);
  }
}
```

Output

```text
    n  NIG alpha  loglik gain  AIC normal  AIC NIG
200    3.281    0.640    594.210    596.930
2000    569.041    0.139    5702.394    5706.117
```

On normal data the NIG runs to the near-normal limit: `alpha` is 3.3 at 200 observations and 569 at 2000 (a very large `alpha` *is* a normal), and the log-likelihood gain over the normal is only 0.64 and 0.14 units, much less than the two points of Akaike's penalty per extra parameter would demand, so the criterion prefers the normal (594 against 597, and 5702 against 5706). The extra parameters cost something and earn nothing, which is what a model-selection criterion is for. In practice this means fitting both and letting the criterion decide, rather than assuming heavy tails because they are fashionable.

## Skewness and kurtosis

Tail weight and skew are summarised by the standardised cumulants: skewness `κ₃/κ₂^1.5` and excess kurtosis `κ₄/κ₂²`, both zero for the normal. A fitted model's `cumulant(order)` gives them without simulation. The program compares the skewness and excess kurtosis of the fitted NIG with those of the sample and of the fitted normal, which is zero by construction.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

#include <vector>

namespace distr = pluss::subsp::stats::distr;
namespace dst = distr::dst;
using namespace pluss::tils;

int main() {
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  const auto x = distr::Nig{1.5, -0.4, 0.1, 0.8}.sample({2000}, f64, 20261022ULL);
  const auto model = distr::Nig{}.MLE(x).model();

  // The cumulants of the fitted model, and the matching sample moments.
  const double k2 = model.cumulant(2).item<double>();
  const double skew_fit = model.cumulant(3).item<double>() / std::pow(k2, 1.5);
  const double kurt_fit = model.cumulant(4).item<double>() / (k2 * k2);
  const double m = x.mean().item<double>();
  const double c2 = ((x - m).pow(2.0)).mean().item<double>();
  const double skew_sample = ((x - m).pow(3.0)).mean().item<double>() / std::pow(c2, 1.5);
  const double kurt_sample = ((x - m).pow(4.0)).mean().item<double>() / (c2 * c2) - 3.0;

  out.writeln("                skewness  excess kurtosis");
  out.writeln("fitted NIG    ", skew_fit, kurt_fit);
  out.writeln("sample        ", skew_sample, kurt_sample);
  out.writeln("fitted normal ", 0.0, 0.0);
}
```

Output

```text
                skewness  excess kurtosis
fitted NIG     -0.722 2.784
sample         -0.780 3.167
fitted normal  0.000 0.000
```

The fitted NIG has a negative skewness (−0.72, the left tail is longer, as `beta < 0` says) and a clearly positive excess kurtosis (2.78), and the sample's figures (−0.78 and 3.17) are close to them, within the sampling error of the higher sample moments, which are notoriously noisy because the few most extreme observations dominate them. The fitted normal has zero of both. The sample cumulants are what a quick diagnostic uses; the fitted ones are smoother because they use the whole density.

## The generalised hyperbolic family and its boundary

The generalised hyperbolic family `Gh{lambda, alpha, beta, location, scale}` has one more parameter than the NIG. Its likelihood is flat along a ridge in the parameters, and its maximum may lie on a boundary where the scale `δ` shrinks to zero (a variance-gamma limit that is not itself a member of the family). The fit keeps `δ` above one thousandth of the sample standard deviation; if the estimate sits at that bound, the sample does not distinguish the family from its limit, and the intervals should not be trusted. The usual remedy is to fix `λ` by using the NIG, as above. The program fits both to the same sample and prints the estimates with the log-likelihood gain, to show how little the fifth parameter buys.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

#include <vector>

namespace distr = pluss::subsp::stats::distr;
namespace dst = distr::dst;
using namespace pluss::tils;

int main() {
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  const auto x = distr::Nig{1.5, -0.4, 0.1, 0.8}.sample({2000}, f64, 20261022ULL);
  const auto nig = distr::Nig{}.MLE(x);
  const auto gh = distr::Gh{}.MLE(x);

  out.writeln("NIG  alpha", nig.alpha, " beta", nig.beta, " location", nig.location, " scale", nig.scale, "  loglik", nig.loglik(x).item<double>());
  out.writeln("GH   lambda", gh.lambda, " alpha", gh.alpha, " beta", gh.beta, " location", gh.location, " scale", gh.scale, "  loglik", gh.loglik(x).item<double>());
  out.writeln("log-likelihood gain of the extra parameter:", gh.loglik(x).item<double>() - nig.loglik(x).item<double>(), "  (Akaike charges 2.0)");
}
```

Output

```text
NIG  alpha 1.745  beta -0.503  location 0.131  scale 0.860   loglik -2160.578
GH   lambda 1.664  alpha 2.571  beta -0.461  location 0.112  scale 0.007   loglik -2159.853
log-likelihood gain of the extra parameter: 0.725   (Akaike charges 2.0)
```

The generalised hyperbolic fit has the higher log-likelihood by construction (it contains the NIG), but the gain is 0.7 units, far below the 2.0 that one extra parameter has to earn. Look at the parameters it uses to get it: `lambda` is 1.66 (the NIG's value is −1/2), `alpha` is 2.57, and the scale is 0.007, essentially at the lower bound of one thousandth of the sample standard deviation. A completely different set of parameters describes the data almost exactly as well: that is the flat ridge of the likelihood, and the reason the individual `Gh` parameters should not be interpreted. The data were generated from a NIG, so the extra parameter has nothing to find. The practical advice stands: **use the NIG unless the sample is large and you have a reason to believe the order differs**, and treat a `Gh` scale at its lower bound as a sign that the extra parameter is not identified.

## Decisions and pitfalls

- **Compare with the normal, always.** Fit both and look at the log-likelihood and Akaike's criterion; a flexible family earns its place only if the criterion says so.
- **Judge by the tail you care about.** Compute the probability of the event that matters (a loss beyond a threshold), not only the likelihood.
- **A smaller `alpha` is a heavier tail.** Large `alpha` is near-normal; `beta` carries the skew, with its sign saying which tail is longer.
- **Prefer the NIG to the `Gh` fit.** The extra parameter is hard to identify; a boundary scale means the interval cannot be trusted.
- **Tail estimates need data in the tail.** With a few hundred observations the extreme tail is an extrapolation; present such numbers as estimates with their uncertainty.

Where next: [Mixtures](mixtures.md) builds multimodal and skewed shapes from simple parts instead.
