<!-- generated from asymmetric_responses.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Asymmetric responses

[Back to Volatility Models](../ts_03_volatility_models.md)

A GARCH(1,1) treats a shock of `+2` and a shock of `−2` identically, because only the *square* of the shock enters the variance. Many series do not behave that way. For equity returns a fall tends to raise volatility more than a rise of the same size (the *leverage effect*); for some commodities and exchange rates the opposite holds. Two models allow the response to depend on the sign of the shock:

- **GJR** adds a term that switches on only for negative shocks: `h_t = ω + (α + γ 1[e_{t−1} < 0]) e²_{t−1} + β h_{t−1}`. The leverage coefficient `γ` is constrained to be non-negative, so a bad shock has total effect `α + γ` and a good shock `α`. The stationarity condition becomes `α + γ/2 + β < 1`.
- **EGARCH** models the *logarithm* of the variance, `log h_t = ω + α (|z_{t−1}| − √(2/π)) + γ z_{t−1} + β log h_{t−1}` with `z = e/√h` the standardised shock. Because the variance is the exponential of the right-hand side it is automatically positive, so `ω`, `α` and `γ` are unrestricted in sign, and `γ` is the asymmetry: a negative `γ` makes bad news raise the variance. The fields are named `arch`, `asym` and `garch`.

Their fit interfaces are `Gjr::fit_mle` and `Egarch::fit_mle`, with vector-valued fields (one entry per lag) such as `fit.asym[0]`. This page fits both to a series with leverage, compares them with the symmetric model, and then shows the case that GJR cannot represent.

## What the asymmetry looks like

The *news impact curve* plots the next variance against the shock that just happened, holding the past variance fixed. It is the quickest way to see what a model says. The program fits GARCH, GJR and EGARCH to 2000 returns generated with strong leverage and prints the next-period variance after shocks from −3 to +3 (in units of the unconditional standard deviation), evaluating each model at its own typical variance.

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

int main() {
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  ts::GjrFit truth;
  truth.mean = 0.0;
  truth.omega = 0.05;
  truth.arch = {0.05};
  truth.asym = {0.20};
  truth.garch = {0.75};
  const Series y = truth.simulate(2000, f64, /*seed=*/4);

  const auto g = ts::Garch11::fit_mle(y, {}, f64);
  const auto j = ts::Gjr::fit_mle(y, {}, f64);
  const auto e = ts::Egarch::fit_mle(y, {}, f64);

  std::cout << std::fixed << std::setprecision(4);
  std::cout << "GARCH(1,1)  omega " << g.omega << "  alpha " << g.alpha << "  beta " << g.beta << "\n";
  std::cout << "GJR         omega " << j.omega << "  alpha " << j.arch[0] << "  gamma " << j.asym[0] << "  beta " << j.garch[0] << "   (true 0.05, 0.05, 0.20, 0.75)\n";
  std::cout << "EGARCH      omega " << e.omega << "  alpha " << e.arch[0] << "  asym " << e.asym[0] << "  beta " << e.garch[0] << "\n\n";

  // News impact: the next variance after a shock s, with the past variance at its unconditional value V.
  const double Vg = g.unconditional_var(), Vj = j.unconditional_var();
  std::cout << std::setprecision(3) << std::setw(8) << "shock" << std::setw(12) << "GARCH" << std::setw(12) << "GJR" << "\n";
  for (double s : {-3.0, -2.0, -1.0, 0.0, 1.0, 2.0, 3.0}) {
    const double eg = s * std::sqrt(Vg), ej = s * std::sqrt(Vj);
    const double hg = g.omega + g.alpha * eg * eg + g.beta * Vg;
    const double hj = j.omega + (j.arch[0] + (ej < 0 ? j.asym[0] : 0.0)) * ej * ej + j.garch[0] * Vj;
    std::cout << std::setw(8) << s << std::setw(12) << hg << std::setw(12) << hj << "\n";
  }
}
```

Output

```text
GARCH(1,1)  omega 0.0799  alpha 0.1658  beta 0.6460
GJR         omega 0.0635  alpha 0.0381  gamma 0.2091  beta 0.7083   (true 0.05, 0.05, 0.20, 0.75)
EGARCH      omega -0.1353  alpha 0.2285  asym -0.1257  beta 0.8534

   shock       GARCH         GJR
  -3.000       0.988       1.314
  -2.000       0.636       0.787
  -1.000       0.425       0.471
   0.000       0.354       0.365
   1.000       0.425       0.382
   2.000       0.636       0.430
   3.000       0.988       0.512
```

The GARCH column is symmetric by construction (the −3 and +3 rows are equal, 0.988). The GJR column is not: a shock of −3 raises next-period variance to 1.31 while +3 raises it only to 0.51, and the gap widens with the size of the shock because the leverage term multiplies the *squared* shock. Even a one-unit shock shows it (0.47 against 0.38). The GJR estimates recover the generating structure: `γ` = 0.209 against 0.20, and the total response to a negative shock `α + γ` = 0.247 against a true 0.25, even though `α` itself (0.038 against 0.05) and `β` (0.708 against 0.75) are off, the usual trade-offs in this likelihood. The EGARCH coefficients are on the log scale and are not comparable with GJR's: its `asym` is negative (−0.126), which is what a leverage effect looks like in that parameterisation, since a negative standardised shock then raises the log-variance.

## Does the asymmetry earn its parameters?

Asymmetric models have an extra parameter each. The log-likelihood always rises with it, so the question is whether it rises *enough*. The program compares the three on the same series with Akaike's and Schwarz's criteria, charging each model for its parameters (four for GARCH(1,1) counting the mean, five for the others).

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

int main() {
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  ts::GjrFit truth;
  truth.mean = 0.0;
  truth.omega = 0.05;
  truth.arch = {0.05};
  truth.asym = {0.20};
  truth.garch = {0.75};
  const double n = 2000.0;

  struct Row { const char* name; double ll; int k; };
  std::cout << std::fixed << std::setprecision(2);
  for (std::uint64_t seed : {4ULL, 5ULL}) {
    const Series y = truth.simulate(2000, f64, seed);
    const Row rows[] = {{"GARCH", ts::Garch11::fit_mle(y, {}, f64).loglik(y), 4},
                        {"GJR", ts::Gjr::fit_mle(y, {}, f64).loglik(y), 5},
                        {"EGARCH", ts::Egarch::fit_mle(y, {}, f64).loglik(y), 5}};
    std::cout << "sample with seed " << seed << "\n" << std::left << std::setw(10) << "model" << std::right << std::setw(12) << "loglik" << std::setw(10) << "AIC" << std::setw(10) << "BIC" << "\n";
    for (const auto& r : rows)
      std::cout << std::left << std::setw(10) << r.name << std::right << std::setw(12) << r.ll << std::setw(10) << -2 * r.ll + 2 * r.k << std::setw(10) << -2 * r.ll + r.k * std::log(n) << "\n";
    std::cout << "\n";
  }
}
```

Output

```text
sample with seed 4
model           loglik       AIC       BIC
GARCH         -1915.66   3839.31   3861.72
GJR           -1895.27   3800.53   3828.54
EGARCH        -1904.25   3818.50   3846.50

sample with seed 5
model           loglik       AIC       BIC
GARCH         -1948.69   3905.39   3927.79
GJR           -1929.08   3868.15   3896.16
EGARCH        -1924.52   3859.04   3887.05
```

The symmetric model has the lowest likelihood and the highest criteria in both samples, by 9 to 39 points of AIC, far more than the penalty for one extra parameter, so the asymmetry is real and either asymmetric model is preferred. Between the two asymmetric models the ranking is not stable: GJR, which generated the data, wins by 18 AIC points in the first sample, and EGARCH wins by 9 in the second. A sample of 2000 observations cannot always tell two asymmetric specifications apart, so choose between them on convenience (GJR is on the variance scale and its coefficients are directly interpretable; EGARCH never needs constraints) and on out-of-sample behaviour, not on one likelihood comparison.

## The opposite sign

GJR constrains `γ ≥ 0`, which encodes the belief that bad news raises volatility more. A series whose volatility responds *more to good news* has no place in that model: the fit sets `γ` to its boundary at zero and returns the symmetric GARCH. EGARCH's `asym` has either sign and handles it. The program simulates an EGARCH series with a *positive* asymmetry and fits both.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

int main() {
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  ts::EgarchFit truth;
  truth.mean = 0.0;
  truth.omega = -0.1;
  truth.arch = {0.15};
  truth.asym = {0.12};  // positive: good news raises the variance more
  truth.garch = {0.9};
  const Series y = truth.simulate(3000, f64, /*seed=*/6);

  const auto e = ts::Egarch::fit_mle(y, {}, f64);
  const auto j = ts::Gjr::fit_mle(y, {}, f64);
  const auto g = ts::Garch11::fit_mle(y, {}, f64);
  std::cout << std::fixed << std::setprecision(4);
  std::cout << "EGARCH asym  " << e.asym[0] << "   (true 0.1200)\n";
  std::cout << "GJR gamma    " << j.asym[0] << "   (cannot go below 0)\n\n";
  std::cout << std::setprecision(2) << "log-likelihood: GARCH " << g.loglik(y) << ", GJR " << j.loglik(y) << ", EGARCH " << e.loglik(y) << "\n";
}
```

Output

```text
EGARCH asym  0.0999   (true 0.1200)
GJR gamma    0.0000   (cannot go below 0)

log-likelihood: GARCH -2850.26, GJR -2850.26, EGARCH -2811.19
```

EGARCH recovers a positive asymmetry (0.100 against 0.12) and has a log-likelihood 39 points above the symmetric model's. GJR's leverage coefficient is exactly zero, at its boundary, and its likelihood is identical to GARCH's (−2850.26): a GJR fit with `γ = 0` *is* a symmetric GARCH. If you do not know the direction of the asymmetry in advance, fit EGARCH, whose sign is free, and look at the sign of `asym` before deciding whether a GJR model is appropriate.

## Decisions and pitfalls

- **Look at the sign first.** Fit EGARCH if you do not know which way the asymmetry goes; a GJR fit with `γ` at zero is a symmetric model in disguise.
- **Do not compare coefficients across models.** GJR is on the variance scale and EGARCH on the log scale; compare likelihoods and criteria.
- **Mind the stationarity conditions.** For GJR the condition is `α + γ/2 + β < 1`, not `α + β < 1`.
- **Asymmetry needs data.** Leverage effects are second-order; with a few hundred observations the improvement is often within noise.
- **Check the residuals of the chosen model**, not just its likelihood: [Checking the fit](checking_the_fit.md).

Where next: [Checking the fit](checking_the_fit.md) tests whether the chosen volatility model has absorbed the clustering.
