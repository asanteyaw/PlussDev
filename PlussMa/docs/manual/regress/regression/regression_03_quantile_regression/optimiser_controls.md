<!-- generated from optimiser_controls.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Optimiser controls

[Back to Quantile Regression](../regression_03_quantile_regression.md)

The check-function loss is not smooth at zero, so it is minimised by a smoothed iteratively reweighted least-squares scheme. At each step the weight of an observation is τ (above the line) or 1 − τ (below it) divided by the larger of its absolute residual and a small constant `eps`; the constant keeps a residual that is exactly zero from receiving an infinite weight. Three options control it.

| Option | Meaning | Default |
|---|---|---|
| `max_iter` | the most iterations to run; must be positive | 500 |
| `tol` | convergence when the largest change in a coefficient is at most `tol` times one plus the largest absolute coefficient; must be positive | 1e-7 |
| `eps` | the floor on the absolute residual in the weights; must be positive | 1e-6 |

The result reports `iterations` and `converged`. A fit that reaches `max_iter` returns with `converged` false and the warning `Quantile IRLS reached max_iter without satisfying tolerance.`; the coefficients are then the last iterate.

## Iterations and tolerance

The median fit of the fan data needs 26 iterations at the defaults. Cutting the budget returns a fit that has not converged, and a looser tolerance stops sooner. The coefficients of an unconverged or loosely converged fit are close to the answer but are not the answer.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  std::vector<double> xs, ys;
  for (int i = 0; i < 60; ++i) {
    const double x = 1.0 + i * 0.25;
    xs.push_back(x);
    const double noise = 2.0 * std::fmod(i * 0.6180339887498949, 1.0) - 1.0;  // a fixed low-discrepancy sequence in [-1, 1)
    ys.push_back(1.0 + 2.0 * x + x * noise);
  }
  const auto x = pluss::make_series(xs, {60, 1}, as_f64());
  const auto y = pluss::make_series(ys, {60}, as_f64());

  out.writeln("by iteration budget");
  for (std::int64_t limit : {1, 3, 10, 500}) {
    reg::QuantileOptions options;
    options.max_iter = limit;
    const auto fit = reg::quantile_fit(y, x, options);
    out.writeln("  max_iter", limit, " iterations", fit.iterations, " converged", fit.converged, " coefficients", fit.coefficients.items<double>(), " objective", fit.objective, " warnings", fit.warnings.size());
  }
  out.writeln("by tolerance");
  const std::vector<std::pair<const char *, double>> tolerances{{"1e-3 ", 1e-3}, {"1e-5 ", 1e-5}, {"1e-7 ", 1e-7}, {"1e-10", 1e-10}};
  for (const auto &[label, tol] : tolerances) {
    reg::QuantileOptions options;
    options.tol = tol;
    const auto fit = reg::quantile_fit(y, x, options);
    out.writeln("  tol", label, " iterations", fit.iterations, " converged", fit.converged, " coefficients", fit.coefficients.items<double>(), " objective", fit.objective);
  }
}
```

Output

```text
by iteration budget
  max_iter 1  iterations 1  converged false  coefficients [0.50274, 2.07951]  objective 123.84521  warnings 1
  max_iter 3  iterations 3  converged false  coefficients [0.64072, 2.06540]  objective 123.74210  warnings 1
  max_iter 10  iterations 10  converged false  coefficients [0.77662, 2.05571]  objective 123.66679  warnings 1
  max_iter 500  iterations 26  converged true  coefficients [0.77655, 2.05572]  objective 123.66678  warnings 0
by tolerance
  tol 1e-3   iterations 9  converged true  coefficients [0.77617, 2.05574]  objective 123.66701
  tol 1e-5   iterations 11  converged true  coefficients [0.77661, 2.05571]  objective 123.66679
  tol 1e-7   iterations 26  converged true  coefficients [0.77655, 2.05572]  objective 123.66678
  tol 1e-10  iterations 27  converged true  coefficients [0.77655, 2.05572]  objective 123.66678
```

One or three iterations are far from the answer (the intercept is 0.50 and 0.64, against 0.78), ten are nearly there with a coefficient of 0.77662 against 0.77655 but not converged, and the default budget converges in 26. Among the tolerances, 10⁻³ stops after 9 iterations with coefficients within 4 × 10⁻⁴ of the final ones, and 10⁻¹⁰ adds only one iteration to the default and changes nothing visible. Each unconverged fit carries the warning.

## The floor `eps`

The exact minimiser of the loss for τ = 0.5 passes through as many observations as the model has parameters (two, here): a vertex of the linear programme that the loss defines. The smoothed iteration approaches that vertex without reaching it, and the residuals of the observations it is approaching shrink toward `eps`. The program lists the three smallest absolute residuals and the objective for several values of `eps`. A smaller `eps` gives a sharper approach to the vertex, at the cost of more iterations. The objective is already correct to many digits at a coarse `eps`, so the choice affects the coefficients far less than it affects the iteration count.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  std::vector<double> xs, ys;
  for (int i = 0; i < 60; ++i) {
    const double x = 1.0 + i * 0.25;
    xs.push_back(x);
    const double noise = 2.0 * std::fmod(i * 0.6180339887498949, 1.0) - 1.0;  // a fixed low-discrepancy sequence in [-1, 1)
    ys.push_back(1.0 + 2.0 * x + x * noise);
  }
  const auto x = pluss::make_series(xs, {60, 1}, as_f64());
  const auto y = pluss::make_series(ys, {60}, as_f64());

  const std::vector<std::pair<const char *, double>> floors{{"1e-2", 1e-2}, {"1e-4", 1e-4}, {"1e-6", 1e-6}, {"1e-9", 1e-9}};
  for (const auto &[label, eps] : floors) {
    reg::QuantileOptions options;
    options.eps = eps;
    const auto fit = reg::quantile_fit(y, x, options);
    auto r = fit.residuals.abs().items<double>();
    std::sort(r.begin(), r.end());
    out.writeln("eps", label, " iterations", fit.iterations, " coefficients", fit.coefficients.items<double>(), " objective", fit.objective, " smallest |residuals|", std::vector<double>(r.begin(), r.begin() + 3));
  }
}
```

Output

```text
eps 1e-2  iterations 13  coefficients [0.77509, 2.05548]  objective 123.66975  smallest |residuals| [0.00250, 0.00272, 0.00478]
eps 1e-4  iterations 16  coefficients [0.77672, 2.05570]  objective 123.66680  smallest |residuals| [0.00008, 0.00008, 0.00043]
eps 1e-6  iterations 26  coefficients [0.77655, 2.05572]  objective 123.66678  smallest |residuals| [0.00000, 0.00000, 0.00056]
eps 1e-9  iterations 27  coefficients [0.77655, 2.05572]  objective 123.66678  smallest |residuals| [0.00000, 0.00000, 0.00056]
```

Two residuals shrink toward zero as `eps` shrinks (0.0025 and 0.0027 at 10⁻², 0.00008 at 10⁻⁴, below 5 × 10⁻⁶ at 10⁻⁶ and 10⁻⁹), while the third smallest stays near 0.0005: the fit is approaching the vertex through two observations, as there are two parameters. A coarse `eps` leaves the answer slightly off (the objective is 123.66975 at 10⁻², against 123.66678), and from 10⁻⁶ on the coefficients and the objective agree to five decimals. The cost of a smaller `eps` is more iterations (13, 16, 26 and 27).

## Invalid settings

`tau` outside (0, 1), and non-positive `alpha`, `rcond`, `max_iter`, `tol` or `eps` (or `alpha` of 1 or more) are rejected before any work is done, with the messages `tau must be in (0,1)`, `alpha must be in (0,1)`, `rcond must be > 0`, `max_iter must be > 0`, `tol must be > 0` and `eps must be > 0`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8}, {8}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{2.1, 3.9, 6.2, 7.8, 10.1, 12.2, 13.8, 16.1}, {8}, as_f64());
  auto attempt = [&](const char *label, auto &&edit) {
    reg::QuantileOptions options;
    edit(options);
    try {
      reg::quantile_fit(y, x, options);
      out.writeln(label, "-> fitted");
    } catch (const std::exception &e) {
      const std::string message = e.what();
      out.writeln(label, "->", message.substr(0, message.find('\n')));
    }
  };
  attempt("tau = 0     ", [](reg::QuantileOptions &o) { o.tau = 0.0; });
  attempt("tau = 1     ", [](reg::QuantileOptions &o) { o.tau = 1.0; });
  attempt("tau = 0.999 ", [](reg::QuantileOptions &o) { o.tau = 0.999; });
  attempt("alpha = 0   ", [](reg::QuantileOptions &o) { o.alpha = 0.0; });
  attempt("rcond = 0   ", [](reg::QuantileOptions &o) { o.rcond = 0.0; });
  attempt("max_iter = 0", [](reg::QuantileOptions &o) { o.max_iter = 0; });
  attempt("tol = 0     ", [](reg::QuantileOptions &o) { o.tol = 0.0; });
  attempt("eps = 0     ", [](reg::QuantileOptions &o) { o.eps = 0.0; });
}
```

Output

```text
tau = 0      -> subsp::stats::regression::quantile_fit: tau must be in (0,1)
tau = 1      -> subsp::stats::regression::quantile_fit: tau must be in (0,1)
tau = 0.999  -> fitted
alpha = 0    -> subsp::stats::regression::quantile_fit: alpha must be in (0,1)
rcond = 0    -> subsp::stats::regression::quantile_fit: rcond must be > 0
max_iter = 0 -> subsp::stats::regression::quantile_fit: max_iter must be > 0
tol = 0      -> subsp::stats::regression::quantile_fit: tol must be > 0
eps = 0      -> subsp::stats::regression::quantile_fit: eps must be > 0
```

Each invalid setting is rejected with its message. The range of `tau` is the open interval (0, 1), so 0.999 is accepted.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="covariance_choices.html">
    <span class="label">Previous</span>
    <span class="title">Covariance choices</span>
  </a>
  <a class="next-link" href="case_weights.html">
    <span class="label">Next</span>
    <span class="title">Case weights</span>
  </a>
</div>
<!-- apiref:nav:end -->
