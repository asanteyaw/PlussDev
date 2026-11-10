<!-- generated from print_glm_summary.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `regression::print_glm_summary`

Writes the report of a fitted generalised linear model to an output stream.

## Signature

```cpp
std::ostream &regression::print_glm_summary(std::ostream &os, const regression::GLMFitResult &fit);
```

## What It Does

Writes exactly the text that `glm_summary` returns to `os` and returns `os`, so the call can be chained or used with any stream: `std::cout`, a file, a string stream.

## Parameters

`os`
: The destination stream.

`fit`
: A completed result from `glm_fit`.

## Returns

The stream `os`.

## Errors

Throws `subsp::stats::regression::print_glm_summary: fit.coefficients is undefined` for a default-constructed result, which has nothing to report.

## Practical Examples

### Example 1: to the standard output

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{1, 0, 2, 3, 2, 5, 4, 8, 9, 14}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Poisson;
  options.predictor_names = {"dose"};
  reg::print_glm_summary(std::cout, reg::glm_fit(y, x, options));
}
```

Output

```text
                            GLM Regression Results
==============================================================================
Dep. Variable:                         y   Family:                          poisson
Model:                               GLM   Link:                                log
Method:                             IRLS   Pseudo R-squared:                 0.8912
Rank:                                  2   No. Iterations:                        6
Alpha:                              0.05   Converged:                          true
No. Observations:                     10   Df Residuals:                          8
Df Model:                              1   Log-Likelihood:                   -16.65
Deviance:                          3.857   AIC:                                37.3
Null Deviance:                     35.45   BIC:                               37.91
==============================================================================
                        coef     std err           z       P>|z|      [0.025      0.975]
------------------------------------------------------------------------------
const                -0.5481      0.5064     -1.0822      0.2792     -1.5407      0.4445
dose                  0.6312      0.1253      5.0383  4.6963e-07      0.3856      0.8767
==============================================================================
Dispersion:                            1   Cond. No.                          7.261
==============================================================================
```

### Example 2: into a string stream

Writing to a string stream gives the same text as `glm_summary`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(0));
  const auto x = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{1, 0, 2, 3, 2, 5, 4, 8, 9, 14}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Poisson;
  const auto fit = reg::glm_fit(y, x, options);
  std::ostringstream stream;
  reg::print_glm_summary(stream, fit);
  out.writeln("same as glm_summary:", stream.str() == reg::glm_summary(fit));
}
```

Output

```text
same as glm_summary: true
```

### Example 3: chaining

The returned stream lets a heading and the report be written in one expression.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{0, 0, 0, 1, 0, 1, 0, 1, 1, 1}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Binomial;
  reg::print_glm_summary(std::cout << "pass/fail on hours of study\n", reg::glm_fit(y, x, options));
}
```

Output

```text
pass/fail on hours of study
                            GLM Regression Results
==============================================================================
Dep. Variable:                         y   Family:                         binomial
Model:                               GLM   Link:                              logit
Method:                             IRLS   Pseudo R-squared:                 0.3746
Rank:                                  2   No. Iterations:                        6
Alpha:                              0.05   Converged:                          true
No. Observations:                     10   Df Residuals:                          8
Df Model:                              1   Log-Likelihood:                   -4.335
Deviance:                           8.67   AIC:                               12.67
Null Deviance:                     13.86   BIC:                               13.28
==============================================================================
                        coef     std err           z       P>|z|      [0.025      0.975]
------------------------------------------------------------------------------
const                -3.7219      2.3479     -1.5852      0.1129     -8.3237      0.8800
x1                    1.3534      0.7958      1.7007      0.0890     -0.2063      2.9132
==============================================================================
Dispersion:                            1   Cond. No.                          7.261
==============================================================================
```

### Example 4: the errors

A default-constructed result has nothing to report, and the call says so.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(0));
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const std::exception &e) {
      const std::string text(e.what());
      out.writeln(text.substr(0, text.find('\n')));
    }
  };
  const reg::GLMFitResult fit;
  report([&] { reg::print_glm_summary(std::cout, fit); });
}
```

Output

```text
subsp::stats::regression::print_glm_summary: fit.coefficients is undefined
```

## See Also

[`glm_summary`](glm_summary.md) returns the text instead of writing it, and [`GLMFitResult`](GLMFitResult.md) holds the numbers.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="glm_summary.html">
    <span class="label">Previous</span>
    <span class="title">glm_summary</span>
  </a>
</div>
<!-- apiref:nav:end -->
