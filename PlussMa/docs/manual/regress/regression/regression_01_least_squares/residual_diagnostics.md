<!-- generated from residual_diagnostics.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Residual diagnostics

[Back to Least Squares](../regression_01_least_squares.md)

The standard errors, *t* tests and intervals of a least-squares fit assume that the errors are independent, have constant variance, and (for exact small-sample inference) are normally distributed. The residuals are the fit's evidence about those assumptions, and the result carries the usual summaries of them.

| Field | Reads | What a departure suggests |
|---|---|---|
| `durbin_watson` | `Σ(eₜ − eₜ₋₁)² / Σeₜ²` over the observations in order | values near 2 mean no first-order serial correlation; near 0, positive correlation; near 4, negative correlation |
| `skewness` | third standardised moment of the residuals | a long tail on one side |
| `kurtosis` | fourth standardised moment (3 for a normal distribution) | heavier or lighter tails than a normal |
| `jarque_bera`, `jarque_bera_pvalue` | `n/6 · (skew² + (kurtosis − 3)²/4)`, upper tail of a chi-squared distribution with 2 degrees of freedom | non-normality |
| `omnibus`, `omnibus_pvalue` | the D'Agostino–Pearson test, built from transformed skewness and kurtosis, chi-squared with 2 degrees of freedom; needs `n ≥ 8` | non-normality, with better small-sample behaviour than Jarque–Bera |

The Durbin–Watson statistic depends on the order of the rows, so it is meaningful only when the rows are in time or space order. The other four do not depend on the order.

## Three residual patterns

The program fits the same straight line to three response series, each the line y = 1 + 0.5 t plus a different disturbance of 24 points: irregular noise in both directions, a slow wave (successive residuals resemble one another), and a skewed disturbance with an occasional large positive value (a fourth power of a cycling index). For the first series it also recomputes the statistics from the residuals.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const std::int64_t n = 24;
  std::vector<double> t, irregular, wave, skewed;
  for (int i = 0; i < n; ++i) {
    t.push_back(i);
    irregular.push_back(1.0 + 0.5 * i + ((i * 7) % 11 - 5) / 10.0);
    wave.push_back(1.0 + 0.5 * i + 0.8 * std::sin(i / 3.0));
    const double r = (i * 7) % 11;
    skewed.push_back(1.0 + 0.5 * i + r * r * r * r / 2000.0);
  }
  const auto x = pluss::make_series(t, {n}, as_f64());

  auto report = [&](const char *label, const std::vector<double> &y) {
    const auto fit = reg::ols_fit(pluss::make_series(y, {n}, as_f64()), x);
    out.writeln(label, "DW", fit.durbin_watson, " skew", fit.skewness, " kurtosis", fit.kurtosis);
    out.writeln("                JB", fit.jarque_bera, " p", fit.jarque_bera_pvalue, " | omnibus", fit.omnibus, " p", fit.omnibus_pvalue);
    return fit;
  };
  const auto irregular_fit = report("irregular noise:", irregular);
  report("slow wave      :", wave);
  report("skewed noise   :", skewed);

  // Recomputing the statistics for the first series from its residuals.
  const auto e = irregular_fit.residuals;
  const auto centred = e.sub(e.mean());
  const double m2 = centred.pow(2.0).mean().item<double>();
  const double skew = centred.pow(3.0).mean().item<double>() / std::pow(m2, 1.5);
  const double kurt = centred.pow(4.0).mean().item<double>() / (m2 * m2);
  const auto later = e.index_select(0, pluss::Series::arange(1, n, 1, pluss::ScalarType::Int64));
  const auto earlier = e.index_select(0, pluss::Series::arange(0, n - 1, 1, pluss::ScalarType::Int64));
  const double dw = later.sub(earlier).pow(2.0).sum().item<double>() / e.pow(2.0).sum().item<double>();
  out.writeln("recomputed     DW", dw, " skew", skew, " kurtosis", kurt, " JB", n / 6.0 * (skew * skew + 0.25 * (kurt - 3.0) * (kurt - 3.0)));
}
```

Output

```text
irregular noise: DW 2.6756  skew -0.0324  kurtosis 1.7517
                JB 1.5624  p 0.4579  | omnibus 4.5325  p 0.1037
slow wave      : DW 0.1255  skew -0.1201  kurtosis 1.7391
                JB 1.6476  p 0.4388  | omnibus 4.8213  p 0.0898
skewed noise   : DW 2.6917  skew 1.4319  kurtosis 3.8981
                JB 9.0082  p 0.0111  | omnibus 10.6612  p 0.0048
recomputed     DW 2.6756  skew -0.0324  kurtosis 1.7517  JB 1.5624
```

Each statistic answers its own question. The irregular noise has a Durbin–Watson value of 2.68 (a little above 2), skewness near zero and kurtosis of 1.75 (light tails, like a uniform distribution), and neither normality test rejects (p = 0.46 and 0.10). The slow wave is flagged by Durbin–Watson alone, at 0.1255, far below 2, because successive residuals resemble one another, while its shape passes both normality tests. The skewed disturbance has skewness 1.43 and kurtosis 3.90, and both tests reject normality (p = 0.011 and 0.005), while Durbin–Watson, at 2.69, does not suspect the order. The last line recomputes Durbin–Watson, skewness, kurtosis and Jarque–Bera for the first series from its residuals, and reproduces them.

## When the rows have no natural order

Shuffling the rows leaves the coefficients, the standard errors, skewness, kurtosis and both normality tests unchanged, because they are functions of the set of residuals. Only the Durbin–Watson statistic changes, since it compares neighbours. So a Durbin–Watson value computed on rows that are not in time order measures nothing in particular.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const std::int64_t n = 24;
  std::vector<double> t, wave;
  for (int i = 0; i < n; ++i) {
    t.push_back(i);
    wave.push_back(1.0 + 0.5 * i + 0.8 * std::sin(i / 3.0));
  }
  // A fixed reordering of the rows: every 5th row first, then the rest (5 and 24 share no factor).
  std::vector<double> t_shuffled, wave_shuffled;
  for (int i = 0; i < n; ++i) {
    const int j = (i * 5) % n;
    t_shuffled.push_back(t[j]);
    wave_shuffled.push_back(wave[j]);
  }
  const auto ordered = reg::ols_fit(pluss::make_series(wave, {n}, as_f64()), pluss::make_series(t, {n}, as_f64()));
  const auto shuffled = reg::ols_fit(pluss::make_series(wave_shuffled, {n}, as_f64()), pluss::make_series(t_shuffled, {n}, as_f64()));
  out.writeln("coefficients ordered ", ordered.coefficients.pull<double>(), " shuffled", shuffled.coefficients.pull<double>());
  out.writeln("skewness     ordered ", ordered.skewness, " shuffled", shuffled.skewness);
  out.writeln("Jarque-Bera  ordered ", ordered.jarque_bera, " shuffled", shuffled.jarque_bera);
  out.writeln("Durbin-Watson ordered", ordered.durbin_watson, " shuffled", shuffled.durbin_watson);
}
```

Output

```text
coefficients ordered  [1.3048, 0.4819]  shuffled [1.3048, 0.4819]
skewness     ordered  -0.1201  shuffled -0.1201
Jarque-Bera  ordered  1.6476  shuffled 1.6476
Durbin-Watson ordered 0.1255  shuffled 2.0388
```

Only the Durbin–Watson value changes under the reordering, from 0.1255 to 2.0388. The coefficients, skewness and Jarque–Bera are identical because they depend on the set of residuals and not on their order.

## Few observations

The omnibus test needs at least eight residuals, and the Jarque–Bera and moment statistics need residuals that are not all equal. Where a statistic does not exist, its field is `NaN` rather than a number that looks plausible, and the p-value is `NaN` with it.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const std::vector<double> t{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  const std::vector<double> y{1.2, 1.9, 3.4, 3.8, 5.3, 5.9, 7.4, 7.8, 9.3, 9.9};
  for (std::int64_t n : {10, 7, 4}) {
    const auto fit = reg::ols_fit(pluss::make_series(std::vector<double>(y.begin(), y.begin() + n), {n}, as_f64()),
                                  pluss::make_series(std::vector<double>(t.begin(), t.begin() + n), {n}, as_f64()));
    out.writeln("n", n, " omnibus", fit.omnibus, " p", fit.omnibus_pvalue, " | JB", fit.jarque_bera, " p", fit.jarque_bera_pvalue, " | DW", fit.durbin_watson);
  }
  // All residuals zero: nothing to measure.
  const auto exact = reg::ols_fit(pluss::make_series(std::vector<double>{3, 5, 7, 9, 11, 13, 15, 17}, {8}, as_f64()), pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8}, {8}, as_f64()));
  out.writeln("exact line: skew", exact.skewness, " kurtosis", exact.kurtosis, " JB", exact.jarque_bera, " omnibus", exact.omnibus, " DW", exact.durbin_watson);
}
```

Output

```text
n 10  omnibus 5.4171  p 0.0666  | JB 1.2373  p 0.5387  | DW 3.7804
n 7  omnibus nan  p nan  | JB 0.8874  p 0.6417  | DW 3.5156
n 4  omnibus nan  p nan  | JB 0.5381  p 0.7641  | DW 3.2448
exact line: skew 0.0000  kurtosis 3.0000  JB nan  omnibus nan  DW nan
```

At ten observations the omnibus statistic is 5.42 (p = 0.067). At seven and four it is `NaN` with its p-value, while Jarque–Bera and Durbin–Watson are still computed; their p-values rest on large-sample theory and should not be trusted with so few residuals. For the exact line the residuals are all zero, so skewness and kurtosis are reported at the normal values (0 and 3), and Jarque–Bera, the omnibus statistic and Durbin–Watson, which divide by a zero spread, are `NaN`.
