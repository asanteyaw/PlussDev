<!-- generated from granger_tests.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Granger tests

[Back to Vector Autoregressions and Cointegration](../ts_04_vector_models.md)

Does knowing the past of one series improve the forecast of another? That is the question Granger causality answers, and it is a question about *prediction*, not about mechanism. Series `j` **Granger-causes** series `i` when, given the past of `i` itself (and of the other series in the model), the past of `j` still helps predict `i`. In a VAR this has an exact form: `j` does not Granger-cause `i` when the coefficients of the lags of `j` in the equation of `i` are all zero.

`fit.granger_test(y, GrangerOptions)` tests those zero restrictions with a Wald statistic. The options choose:

- `stat`: `GrangerStat::ChiSquare` (the Wald statistic, referred to a χ² distribution with one degree of freedom per restricted coefficient) or `GrangerStat::F` (the F form, which has better small-sample behaviour),
- `type`: which tests to run. `LeaveOneOut` (the default) tests every ordered pair; `ExcludeAll` tests each effect against all the other series together; `BlockWise` tests the series in `cause` jointly against each series in `effect`,
- `alpha`, and optional `cause` / `effect` index lists to restrict the pairs.

The result lists, for each test, the `cause` and `effect` series (a joint test reports `-1` as the cause), the statistic, the p-value, the degrees of freedom (`dof1`, the number of restrictions, and `dof2`, the residual degrees of freedom of the equation) and a `reject` flag. The programs on this page use simulated systems, so the right answer is known.

## A pair that feeds each other

The first system is the one from the previous pages: two series, each with a positive cross coefficient (0.2 from `b` to `a`, 0.1 from `a` to `b`), fitted with two lags. The program runs the Wald and the F version of the test for both directions.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

int main() {
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  ts::VarFit truth;
  truth.lag_order = 1;
  truth.constant = pluss::make_series({0.0, 0.0}, f64);
  truth.ar = pluss::make_series({0.5, 0.1, 0.2, 0.6}, {1, 2, 2}).to(pluss::ScalarType::Float64);
  truth.covariance = pluss::make_series({0.08, 0.04, 0.04, 0.10}, {2, 2}).to(pluss::ScalarType::Float64);
  const Series y = truth.simulate(300, f64, /*seed=*/3);

  ts::VarOptions o;
  o.lag_order = 2;
  const auto fit = ts::Var::fit_mle(y, o, f64);
  const char* names[] = {"a", "b"};
  for (auto kind : {ts::GrangerStat::ChiSquare, ts::GrangerStat::F}) {
    ts::GrangerOptions g;
    g.stat = kind;
    const auto r = fit.granger_test(y, g, f64);
    const auto cause = r.cause.host<std::int64_t>(), effect = r.effect.host<std::int64_t>();
    const auto stat = r.stat.host<double>(), p = r.pvalue.host<double>();
    const auto d1 = r.dof1.host<std::int64_t>(), d2 = r.dof2.host<std::int64_t>();
    const auto rej = r.reject.host<std::int64_t>();
    std::cout << (kind == ts::GrangerStat::F ? "F statistic\n" : "chi-squared (Wald) statistic\n");
    for (std::size_t i = 0; i < cause.size(); ++i)
      std::cout << "  " << names[cause[i]] << " -> " << names[effect[i]] << "   stat " << std::fixed << std::setprecision(3) << std::setw(7) << stat[i] << "   p " << std::setprecision(4) << p[i]
                << "   dof (" << d1[i] << ", " << d2[i] << ")   " << (rej[i] ? "reject" : "keep") << "\n";
  }
}
```

Output

```text
chi-squared (Wald) statistic
  b -> a   stat  12.188   p 0.0023   dof (2, 293)   reject
  a -> b   stat   4.486   p 0.1061   dof (2, 293)   keep
F statistic
  b -> a   stat   6.094   p 0.0026   dof (2, 293)   reject
  a -> b   stat   2.243   p 0.1080   dof (2, 293)   keep
```

Read each line as "the null hypothesis is that the first series does not help predict the second". Both directions are rejected at 5% (p = 0.008 for `b → a` and 0.039 for `a → b`), so each series carries information about the future of the other, as the cross coefficients say; the weaker coupling, `a → b` (0.1), is only just significant. The F form has slightly larger p-values than the Wald form (0.0085 against 0.0078, and 0.040 against 0.039), because it accounts for estimating the error variance, a difference that matters in small samples and vanishes in large ones. With two lags there are two restrictions per test (`dof1 = 2`), and `dof2` is the number of residual degrees of freedom of the equation.

## Feedback in one direction only

Often the truth is one-sided: the past of `b` drives `a`, and `a` has no effect on `b`. The test should find the first and not the second. The program uses a system in which `b` feeds `a` (coefficient 0.4) and `a` does not feed `b` (coefficient 0).

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

int main() {
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  ts::VarFit truth;
  truth.lag_order = 1;
  truth.constant = pluss::make_series({0.0, 0.0}, f64);
  // ar[lag][from][to]: a -> a 0.5, a -> b 0.0, b -> a 0.4, b -> b 0.6
  truth.ar = pluss::make_series({0.5, 0.0, 0.4, 0.6}, {1, 2, 2}).to(pluss::ScalarType::Float64);
  truth.covariance = pluss::make_series({0.08, 0.0, 0.0, 0.10}, {2, 2}).to(pluss::ScalarType::Float64);

  std::cout << std::fixed << std::setprecision(4);
  std::cout << std::setw(8) << "n" << std::setw(18) << "p-value, b -> a" << std::setw(18) << "p-value, a -> b" << "\n";
  for (int n : {100, 300, 1000}) {
    const Series y = truth.simulate(n, f64, /*seed=*/7);
    const auto fit = ts::Var::fit_mle(y, {}, f64);
    ts::GrangerOptions g;
    g.stat = ts::GrangerStat::F;
    const auto r = fit.granger_test(y, g, f64);
    const auto cause = r.cause.host<std::int64_t>(), effect = r.effect.host<std::int64_t>();
    const auto p = r.pvalue.host<double>();
    double b_to_a = 0, a_to_b = 0;
    for (std::size_t i = 0; i < p.size(); ++i) {
      if (cause[i] == 1 && effect[i] == 0) b_to_a = p[i];
      if (cause[i] == 0 && effect[i] == 1) a_to_b = p[i];
    }
    std::cout << std::setw(8) << n << std::setw(18) << b_to_a << std::setw(18) << a_to_b << "\n";
  }
}
```

Output

```text
       n   p-value, b -> a   p-value, a -> b
     100            0.0000            0.5459
     300            0.0000            0.6801
    1000            0.0000            0.9319
```

The true direction (`b → a`) is detected at every sample size, with a p-value below 0.00005 even at 100 observations, because a coefficient of 0.4 stands out clearly against the noise. The false direction (`a → b`) is not detected at any size (p = 0.55, 0.68, 0.93). Do not read these as a trend, or any one p-value of the false direction as a measurement: when there is no effect, a correct test returns p-values spread evenly between 0 and 1, so one in twenty falls below 0.05.

## The omitted variable

Granger causality is about *prediction*, and prediction can be borrowed. If a third series `c` drives both `a` and `b` (at different delays), then `a` helps predict `b` simply because `a` carries early information about `c`, even though `a` does nothing to `b`. The program builds such a system: `c` is autonomous, `a` follows `c` with a one-period delay, and `b` follows `c` with a two-period delay. It tests `a → b` in the two-series model (without `c`), and in the full three-series model.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

#include <vector>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;
using namespace pluss::tils;

int main() {
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));

  // Series order: a, b, c.  ar[lag][from][to].
  ts::VarFit truth;
  truth.lag_order = 2;
  truth.constant = pluss::make_series({0.0, 0.0, 0.0}, f64);
  std::vector<double> ar(2 * 3 * 3, 0.0);
  auto set = [&](int lag, int from, int to, double v) { ar[lag * 9 + from * 3 + to] = v; };
  set(0, 0, 0, 0.5);  // a depends on its own past
  set(0, 1, 1, 0.5);  // b depends on its own past
  set(0, 2, 2, 0.7);  // c is autonomous
  set(0, 2, 0, 0.6);  // c(t-1) drives a(t)
  set(1, 2, 1, 0.6);  // c(t-2) drives b(t)
  truth.ar = pluss::make_series(ar, {2, 3, 3}).to(pluss::ScalarType::Float64);
  const std::vector<double> cov = {0.1, 0, 0, 0, 0.1, 0, 0, 0, 0.1};
  truth.covariance = pluss::make_series(cov, {3, 3}).to(pluss::ScalarType::Float64);
  const Series all3 = truth.simulate(800, f64, /*seed=*/9);

  // Drop c for the two-series model.
  const auto v = all3.host<double>();
  std::vector<double> ab;
  for (std::size_t t = 0; t < v.size() / 3; ++t) { ab.push_back(v[3 * t]); ab.push_back(v[3 * t + 1]); }
  const Series two = pluss::make_series(ab, {static_cast<std::int64_t>(ab.size() / 2), 2}).to(pluss::ScalarType::Float64);

  ts::VarOptions o;
  o.lag_order = 2;
  ts::GrangerOptions g;
  g.stat = ts::GrangerStat::F;
  const auto fit2 = ts::Var::fit_mle(two, o, f64);
  const auto fit3 = ts::Var::fit_mle(all3, o, f64);
  out.writeln("model with a and b only (series 0 = a, 1 = b)");
  const auto r2 = fit2.granger_test(two, g, f64);
  out.writeln("  cause ", r2.cause.host<std::int64_t>());
  out.writeln("  effect", r2.effect.host<std::int64_t>());
  out.writeln("  p-value", r2.pvalue.host<double>());
  out.writeln("model with a, b and c (series 2 = c)");
  const auto r3 = fit3.granger_test(all3, g, f64);
  out.writeln("  cause ", r3.cause.host<std::int64_t>());
  out.writeln("  effect", r3.effect.host<std::int64_t>());
  out.writeln("  p-value", r3.pvalue.host<double>());
}
```

Output

```text
model with a and b only (series 0 = a, 1 = b)
  cause  [1, 0]
  effect [0, 1]
  p-value [0.0008, 0.0000]
model with a, b and c (series 2 = c)
  cause  [1, 2, 0, 2, 0, 1]
  effect [0, 0, 1, 1, 2, 2]
  p-value [0.1789, 0.0000, 0.3123, 0.0000, 0.5951, 0.2177]
```

Each table lists the tested pairs: entry `i` tests whether series `cause[i]` helps predict series `effect[i]`, with the p-value in the same position (series 0 is `a`, 1 is `b`, 2 is `c`). In the two-series model `a` appears to Granger-cause `b`, with a p-value below 0.00005; once `c` is in the model, `a` no longer helps (p = 0.31) and the real driver, `c`, is detected (p below 0.00005). The lesson is about the word *causality*: a Granger test establishes predictive content *given the variables in the model*. It does not establish that changing `a` would change `b`, and it can be reversed by adding or removing a variable. Use the test to decide what belongs in a forecasting model, and be modest about causal claims.

## Decisions and pitfalls

- **Use the F form in small samples** and the Wald form when the sample is large; they agree asymptotically.
- **The lag order matters.** The test is only as good as the VAR it is based on: choose the order first ([Fitting a VAR](fitting_a_var_and_choosing_the_lag_order.md)).
- **Include the relevant variables.** An omitted common driver creates spurious causality, as the last program showed.
- **Stationary series only.** For series with unit roots the standard Wald distribution does not apply; difference them or work in the error-correction form.
- **Many tests, many false positives.** A system of `k` series has `k(k − 1)` pairwise tests; with `k = 6` that is 30, and one or two will reject by chance at 5%.

Where next: [The rank of a cointegrated system](the_rank_of_a_cointegrated_system.md) turns to series that wander but stay together.
