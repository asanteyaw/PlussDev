<!-- generated from impulse_responses_and_variance_decompositions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Impulse responses and variance decompositions

[Back to Vector Autoregressions and Cointegration](../ts_04_vector_models.md)

The coefficients of a VAR are hard to read directly: with two series and two lags there are eight of them, and they interact. Two derived tools answer the questions people actually ask.

- The **impulse response function** (IRF) traces what happens to every series over the following periods after a one-standard-deviation shock to one series: how big the effect is, how long it lasts, and where it goes.
- The **forecast-error variance decomposition** (FEVD) asks, for each series and horizon, what fraction of the uncertainty in its forecast comes from each shock.

Both come from writing the VAR as an infinite moving average of past shocks, `y_t = Σ Ψ_s e_{t−s}`. The matrices `Ψ_s` give the effect of a shock `s` periods ago. The complication is that the shocks `e_t` are correlated (the covariance `Σ` is not diagonal), so "a shock to series 0 alone" is not well defined: when series 0 is hit, series 1 usually moves too. There are two standard resolutions, and `IrfOptions::method` and `FevdOptions::method` select between them.

- **Orthogonalised** (`IrfMethod::Orthogonalized`, the default): take the Cholesky factor `P` of `Σ` (`Σ = P P'`) and use the uncorrelated shocks `P⁻¹ e`. The first series is treated as not reacting to the others within the same period, and the second as reacting to the first but not to the later ones, and so on. The result **depends on the order of the columns of `y`**, which is an identifying assumption, not a technicality.
- **Generalised** (`IrfMethod::Generalized`): shock series `j` alone and let the other series respond through the correlation in `Σ`, scaled by `1/√σ_jj`. It does not depend on the ordering, but the shocks it describes overlap, so its variance shares are normalised to add up to one.

`fit.irf(options)` returns an `h × k × k` array: entry `[s][j][i]` is the response of series `i`, `s` steps after a shock to series `j` (step 0 is the impact). `fit.fevd(options)` has the same layout, with entry `[s][j][i]` the share of series `i`'s forecast-error variance, `s + 1` steps ahead, caused by shock `j`.

The programs fit the two-series system of the previous page (`a_t = 0.5 a_{t−1} + 0.2 b_{t−1} + e_a`, `b_t = 0.1 a_{t−1} + 0.6 b_{t−1} + e_b`, with correlated shocks) to 600 simulated observations.

## Impulse responses

The program prints the response of both series to a shock in each, under both methods.

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

  ts::VarFit truth;
  truth.lag_order = 1;
  truth.constant = pluss::make_series({0.0, 0.0}, f64);
  truth.ar = pluss::make_series({0.5, 0.1, 0.2, 0.6}, {1, 2, 2}).to(pluss::ScalarType::Float64);
  truth.covariance = pluss::make_series({0.08, 0.04, 0.04, 0.10}, {2, 2}).to(pluss::ScalarType::Float64);
  const Series y = truth.simulate(600, f64, /*seed=*/5);
  const auto fit = ts::Var::fit_mle(y, {}, f64);

  // Each step holds four numbers: the shock to a moving (a, b), then the shock to b moving (a, b).
  ts::IrfOptions io;
  io.horizon = 7;
  const auto ortho = fit.irf(io, f64).host<double>();
  io.method = ts::IrfMethod::Generalized;
  const auto general = fit.irf(io, f64).host<double>();

  out.writeln("orthogonalised responses: [shock to a -> a, b | shock to b -> a, b]");
  for (std::size_t s = 0; s < ortho.size() / 4; ++s) out.writeln("  step", s, std::vector<double>(ortho.begin() + 4 * s, ortho.begin() + 4 * s + 4));
  out.writeln("generalised responses");
  for (std::size_t s = 0; s < general.size() / 4; ++s) out.writeln("  step", s, std::vector<double>(general.begin() + 4 * s, general.begin() + 4 * s + 4));
}
```

Output

```text
orthogonalised responses: [shock to a -> a, b | shock to b -> a, b]
  step 0 [0.2817, 0.1274, 0.0000, 0.2728]
  step 1 [0.1462, 0.1011, 0.0638, 0.1595]
  step 2 [0.0840, 0.0729, 0.0636, 0.0993]
  step 3 [0.0518, 0.0506, 0.0495, 0.0640]
  step 4 [0.0332, 0.0344, 0.0354, 0.0421]
  step 5 [0.0218, 0.0233, 0.0245, 0.0280]
  step 6 [0.0144, 0.0157, 0.0167, 0.0187]
generalised responses
  step 0 [0.2817, 0.1274, 0.1192, 0.3011]
  step 1 [0.1462, 0.1011, 0.1196, 0.1873]
  step 2 [0.0840, 0.0729, 0.0932, 0.1208]
  step 3 [0.0518, 0.0506, 0.0668, 0.0794]
  step 4 [0.0332, 0.0344, 0.0461, 0.0527]
  step 5 [0.0218, 0.0233, 0.0314, 0.0352]
  step 6 [0.0144, 0.0157, 0.0212, 0.0235]
```

Start with step 0. In the orthogonalised table a shock to series `a` moves `a` by about 0.28 (the standard deviation of its own error) and also moves `b` on impact, because `b`'s error is correlated with `a`'s; a shock to `b` does *not* move `a` on impact (the ordering assumption: `a` is first). In the generalised table the shock to `b` does move `a` on impact, through the correlation, which is the difference between the two methods. After step 0 every response decays geometrically, as it must for a stationary VAR: the system returns to its mean. The speed of the decay is the persistence of the system; the cross responses (a shock to `b` raising `a` over the next steps) are the cross coefficients at work.

## Does the ordering matter?

The orthogonalised responses depend on the column order; the generalised ones do not. The program refits the same data with the two columns swapped and prints the impact response of `b` to a shock in `a`, which is the quantity the ordering decides.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

int main() {
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  ts::VarFit truth;
  truth.lag_order = 1;
  truth.constant = pluss::make_series({0.0, 0.0}, f64);
  truth.ar = pluss::make_series({0.5, 0.1, 0.2, 0.6}, {1, 2, 2}).to(pluss::ScalarType::Float64);
  truth.covariance = pluss::make_series({0.08, 0.04, 0.04, 0.10}, {2, 2}).to(pluss::ScalarType::Float64);
  const Series y = truth.simulate(600, f64, /*seed=*/5);

  const auto v = y.host<double>();
  std::vector<double> swapped(v.size());
  for (std::size_t t = 0; t < v.size() / 2; ++t) { swapped[2 * t] = v[2 * t + 1]; swapped[2 * t + 1] = v[2 * t]; }
  const Series y_swapped = pluss::make_series(swapped, {static_cast<std::int64_t>(v.size() / 2), 2}).to(pluss::ScalarType::Float64);

  const auto fit_ab = ts::Var::fit_mle(y, {}, f64);
  const auto fit_ba = ts::Var::fit_mle(y_swapped, {}, f64);

  std::cout << std::fixed << std::setprecision(4);
  std::cout << std::setw(30) << "" << std::setw(16) << "order (a, b)" << std::setw(16) << "order (b, a)" << "\n";
  for (auto method : {ts::IrfMethod::Orthogonalized, ts::IrfMethod::Generalized}) {
    ts::IrfOptions io;
    io.horizon = 1;
    io.method = method;
    const auto r_ab = fit_ab.irf(io, f64).host<double>();  // [step 0][shock j][response i]: columns are a, b
    const auto r_ba = fit_ba.irf(io, f64).host<double>();  // columns are b, a
    std::cout << std::left << std::setw(30) << (method == ts::IrfMethod::Orthogonalized ? "orthogonalised" : "generalised") << std::right << "\n";
    std::cout << std::left << std::setw(30) << "  response of b to a shock to a" << std::right << std::setw(16) << r_ab[1] << std::setw(16) << r_ba[2] << "\n";
    std::cout << std::left << std::setw(30) << "  response of a to a shock to b" << std::right << std::setw(16) << r_ab[2] << std::setw(16) << r_ba[1] << "\n";
  }
}
```

Output

```text
                                  order (a, b)    order (b, a)
orthogonalised                
  response of b to a shock to a          0.1274          0.0000
  response of a to a shock to b          0.0000          0.1192
generalised                   
  response of b to a shock to a          0.1274          0.1274
  response of a to a shock to b          0.1192          0.1192
```

With the orthogonalised method the answers change when the order changes: placing `a` first says `a`'s shock moves `b` on impact and not the reverse, and placing `b` first says the opposite. With the generalised method both orders give identical numbers. So the choice is a modelling decision: use the orthogonalised responses when there is a natural causal ordering (a policy rate that moves first, a market that leads another), and the generalised responses when there is none; and if the two disagree materially, the correlation between the shocks is carrying the story, and you should say so.

## Variance decompositions

The FEVD turns the same moving-average representation into shares. For series `i` at horizon `h` it reports which fraction of the variance of the `h`-step forecast error comes from each shock. At horizon 1 the shares are determined by the covariance alone; as the horizon grows the shares converge to the long-run contribution of each shock. The program prints the shares for each series at several horizons, for both methods.

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
  const Series y = truth.simulate(600, f64, /*seed=*/5);
  const auto fit = ts::Var::fit_mle(y, {}, f64);

  std::cout << std::fixed << std::setprecision(3);
  for (auto method : {ts::IrfMethod::Orthogonalized, ts::IrfMethod::Generalized}) {
    ts::FevdOptions fo;
    fo.horizon = 10;
    fo.method = method;
    const auto v = fit.fevd(fo, f64).host<double>();
    std::cout << (method == ts::IrfMethod::Orthogonalized ? "orthogonalised shares" : "generalised shares (normalised)") << "\n";
    std::cout << std::setw(8) << "horizon" << "   variance of a: from a, from b   variance of b: from a, from b" << "\n";
    for (int s : {0, 1, 4, 9}) {
      // entry [s][j][i] = share of series i caused by shock j
      std::cout << std::setw(8) << s + 1 << std::setw(18) << v[4 * s + 0] << std::setw(9) << v[4 * s + 2] << std::setw(22) << v[4 * s + 1] << std::setw(9) << v[4 * s + 3] << "\n";
    }
    std::cout << "\n";
  }
}
```

Output

```text
orthogonalised shares
 horizon   variance of a: from a, from b   variance of b: from a, from b
       1             1.000    0.000                 0.179    0.821
       2             0.961    0.039                 0.209    0.791
       5             0.904    0.096                 0.235    0.765
      10             0.897    0.103                 0.238    0.762

generalised shares (normalised)
 horizon   variance of a: from a, from b   variance of b: from a, from b
       1             0.848    0.152                 0.152    0.848
       2             0.779    0.221                 0.174    0.826
       5             0.718    0.282                 0.192    0.808
      10             0.712    0.288                 0.194    0.806
```

At horizon 1 the orthogonalised shares of series `a` are 1 and 0 by construction (it is first, so it is its own shock only), while series `b`'s variance is split between the shock correlated with `a` and its own. As the horizon grows `b`'s share from `a`'s shock rises because `a` feeds `b` through the cross coefficient. The generalised shares, normalised to add to one, are less extreme at horizon 1, because the common part of the shocks is not attributed to the first series. The shares are the answer to "what drives the uncertainty of this series at this horizon?", and they are only as good as the identification (the ordering) behind the orthogonalised version.

## Decisions and pitfalls

- **State the ordering.** Orthogonalised responses are a statement about which series moves first in a period; report it with the results, and show the generalised version as a check.
- **Shocks are one standard deviation**, not one unit. Compare the sizes of responses across shocks only after allowing for that.
- **The VAR must be stable.** The responses of a unit-root system do not die out; difference the series or use the error-correction form ([The error-correction model](the_error_correction_model.md)).
- **Responses are estimates.** Like any fitted quantity they carry sampling error; with a small sample a response that looks like zero may not be.
- **Index carefully.** The layout is `[step][shock][response]`; transposing it swaps cause and effect.

Where next: [Granger tests](granger_tests.md) asks the question in a testable form: does one series help predict another?
