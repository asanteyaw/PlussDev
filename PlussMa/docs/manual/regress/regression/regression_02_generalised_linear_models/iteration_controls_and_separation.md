<!-- generated from iteration_controls_and_separation.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Iteration controls and separation

[Back to Generalised Linear Models](../regression_02_generalised_linear_models.md)

IRLS starts from a simple guess and improves the coefficients until the largest change in any coefficient is no more than `tol · (1 + largest |coefficient|)`. Two options control it, and two result fields report what happened.

| Name | Meaning | Default |
|---|---|---|
| `GLMOptions::max_iter` | the most iterations to run; must be positive | 200 |
| `GLMOptions::tol` | the convergence tolerance, relative to the size of the coefficients; must be positive | 1e-8 |
| `GLMFitResult::iterations` | iterations actually run | |
| `GLMFitResult::converged` | whether the tolerance was met before `max_iter` | |

When the tolerance is not met, the fit still returns, with `converged` false and the warning `IRLS reached max_iter without satisfying tolerance.` in `warnings`. The coefficients are then the last iterate and nothing more.

## Stopping early

The program fits the study-hours logistic model with `max_iter` from 1 to 200. The estimates approach their limit quickly, as is typical of IRLS, and the fit only reports success once the change is within the tolerance.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto hours = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto passed = pluss::make_series(std::vector<double>{0, 0, 0, 1, 0, 1, 0, 1, 1, 1}, {10}, as_f64());
  for (std::int64_t limit : {1, 2, 3, 5, 6, 200}) {
    reg::GLMOptions options;
    options.family = reg::GLMFamily::Binomial;
    options.max_iter = limit;
    const auto fit = reg::glm_fit(passed, hours, options);
    out.writeln("max_iter", limit, " iterations", fit.iterations, " converged", fit.converged, " coefficients", fit.coefficients.pull<double>(), " warnings", fit.warnings.size());
  }
}
```

Output

```text
max_iter 1  iterations 1  converged false  coefficients [-2.53333, 0.92121]  warnings 1
max_iter 2  iterations 2  converged false  coefficients [-3.45581, 1.25666]  warnings 1
max_iter 3  iterations 3  converged false  coefficients [-3.70608, 1.34767]  warnings 1
max_iter 5  iterations 5  converged false  coefficients [-3.72188, 1.35341]  warnings 1
max_iter 6  iterations 6  converged true  coefficients [-3.72188, 1.35341]  warnings 0
max_iter 200  iterations 6  converged true  coefficients [-3.72188, 1.35341]  warnings 0
```

After 5 iterations the coefficients already agree with their final values to five decimals, but `converged` becomes true only at iteration 6: the criterion looks at the size of the last change, so it needs one further small step to certify the fit. The runs cut short carry one warning each, the converged runs none, and with `max_iter = 200` the fit stops at 6.

## The tolerance

A looser `tol` stops earlier and a tighter one runs further. The convergence of IRLS is quadratic near the solution, so even a very much tighter tolerance costs only a few more iterations.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const auto hours = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto passed = pluss::make_series(std::vector<double>{0, 0, 0, 1, 0, 1, 0, 1, 1, 1}, {10}, as_f64());
  const std::vector<std::pair<const char *, double>> tolerances{{"1e-2 ", 1e-2}, {"1e-4 ", 1e-4}, {"1e-8 ", 1e-8}, {"1e-12", 1e-12}};
  for (const auto &[label, tol] : tolerances) {
    reg::GLMOptions options;
    options.family = reg::GLMFamily::Binomial;
    options.tol = tol;
    const auto fit = reg::glm_fit(passed, hours, options);
    out.writeln("tol", label, " iterations", fit.iterations, " converged", fit.converged, " coefficients", fit.coefficients.pull<double>());
  }
}
```

Output

```text
tol 1e-2   iterations 4  converged true  coefficients [-3.72182364, 1.35339041]
tol 1e-4   iterations 5  converged true  coefficients [-3.72188168, 1.35341152]
tol 1e-8   iterations 6  converged true  coefficients [-3.72188168, 1.35341152]
tol 1e-12  iterations 7  converged true  coefficients [-3.72188168, 1.35341152]
```

The iteration count rises by one as the tolerance goes from 10⁻² to 10⁻⁴, from 10⁻⁴ to 10⁻⁸, and from 10⁻⁸ to 10⁻¹²: the convergence is quadratic near the solution, so tightening the tolerance by many orders of magnitude costs an iteration or two. The coefficient at 10⁻² differs from the final value only in the fifth decimal.

## Separation

In a binomial model, if some combination of the predictors perfectly divides the successes from the failures, the likelihood keeps increasing as the coefficients grow, and no finite maximum exists. This is *separation*. IRLS does not find a solution: the slope grows at every step and the fit ends at `max_iter`. The signs are always the same: `converged` is false, the coefficients are large, the deviance is essentially zero, and the standard errors are enormous.

The first data set below has every failure to the left of every success. The second has a single success among the failures, so the classes overlap slightly and the maximum is finite.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto hours = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Binomial;

  auto report = [&](const char *label, const std::vector<double> &outcome) {
    const auto fit = reg::glm_fit(pluss::make_series(outcome, {10}, as_f64()), hours, options);
    out.writeln(label, "converged", fit.converged, " iterations", fit.iterations, " coefficients", fit.coefficients.pull<double>());
    out.writeln("      deviance", fit.deviance, " std errors", fit.std_errors.pull<double>(), " warnings", fit.warnings.size());
  };
  report("perfectly separated   :", {0, 0, 0, 0, 0, 1, 1, 1, 1, 1});
  report("one overlapping point :", {0, 0, 0, 0, 1, 0, 1, 1, 1, 1});

  // More iterations only move the divergent coefficients further out.
  for (std::int64_t limit : {20, 50, 200, 1000}) {
    reg::GLMOptions more = options;
    more.max_iter = limit;
    const auto fit = reg::glm_fit(pluss::make_series(std::vector<double>{0, 0, 0, 0, 0, 1, 1, 1, 1, 1}, {10}, as_f64()), hours, more);
    out.writeln("separated, max_iter", limit, " slope", fit.coefficients.pull<double>()[1], " converged", fit.converged);
  }
}
```

Output

```text
perfectly separated   : converged false  iterations 200  coefficients [-513.6414, 186.7787]
      deviance 0.0000  std errors [68313.0047, 22019.2748]  warnings 1
one overlapping point : converged true  iterations 8  coefficients [-7.1590, 2.6033]
      deviance 5.0180  std errors [4.7594, 1.6801]  warnings 0
separated, max_iter 20  slope 67.4905  converged false
separated, max_iter 50  slope 95.8696  converged false
separated, max_iter 200  slope 186.7787  converged false
separated, max_iter 1000  slope 671.6272  converged false
```

The separated data end at 200 iterations with `converged` false, coefficients of −513.6 and 186.8, a deviance of zero at the digits shown, and standard errors of 68,313 and 22,019: nonsense that is the signature of separation. With one overlapping point the fit converges in 8 iterations to finite coefficients (−7.159 and 2.603) with a deviance of 5.02 and standard errors of 4.76 and 1.68. The last four lines show the slope of the separated fit growing with every extra iteration allowed (67.5, 95.9, 186.8, 671.6) and never settling.

There is nothing to fix in the iteration. The data are telling you that the predictor is a perfect classifier, and the fitted odds ratio is not a number the data can estimate. Report the separation, collect more data near the boundary, or use fewer or coarser predictors.
