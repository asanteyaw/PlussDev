<!-- generated from print_ols_summary.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `regression::print_ols_summary`

Writes the report of a fitted least-squares model to an output stream.

## Signature

```cpp
std::ostream &regression::print_ols_summary(std::ostream &os, const regression::OLSFitResult &fit);
```

## What It Does

Writes exactly the text that `ols_summary` returns to `os` and returns `os`, so the call can be chained or used with any stream: `std::cout`, a file, a string stream.

## Parameters

`os`
: The destination stream.

`fit`
: A completed result from `ols_fit`.

## Returns

The stream `os`.

## Errors

Throws `subsp::stats::regression::print_ols_summary: fit.coefficients is undefined` for a default-constructed result, which has nothing to report.

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
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8}, {8}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{2.3, 4.1, 6.2, 7.9, 10.2, 12.1, 13.8, 16.2}, {8}, as_f64());
  reg::print_ols_summary(std::cout, reg::ols_fit(y, x));
}
```

Output

```text
                            OLS Regression Results
==============================================================================
Dep. Variable:                         y   R-squared:                         0.999
Model:                               OLS   Adj. R-squared:                   0.9988
Method:                    Least Squares   F-statistic:                        5735
Rank:                                  2   Prob (F-statistic):             3.569e-10
Alpha:                              0.05   Log-Likelihood:                    4.026
No. Observations:                      8   AIC:                              -4.052
Df Residuals:                          6   BIC:                              -3.893
Df Model:                              1   Covariance Type:               nonrobust
==============================================================================
                        coef     std err           t       P>|t|      [0.025      0.975]
------------------------------------------------------------------------------
const                 0.2179      0.1316      1.6552      0.1490     -0.1042      0.5399
x1                    1.9738      0.0261     75.7296  3.5688e-10      1.9100      2.0376
==============================================================================
Omnibus:                            1.16   Durbin-Watson:                      2.86
Prob(Omnibus):                    0.5598   Jarque-Bera (JB):                 0.7909
Skew:                            -0.5117   Prob(JB):                         0.6734
Kurtosis:                          1.849   Cond. No.                          11.48
==============================================================================
```

### Example 2: into a string stream

Writing to a string stream gives the same text as `ols_summary`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(0));
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8}, {8}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{2.3, 4.1, 6.2, 7.9, 10.2, 12.1, 13.8, 16.2}, {8}, as_f64());
  const auto fit = reg::ols_fit(y, x);
  std::ostringstream stream;
  reg::print_ols_summary(stream, fit);
  out.writeln("same as ols_summary:", stream.str() == reg::ols_summary(fit));
}
```

Output

```text
same as ols_summary: true
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
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8}, {8}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{2.3, 4.1, 6.2, 7.9, 10.2, 12.1, 13.8, 16.2}, {8}, as_f64());
  reg::print_ols_summary(std::cout << "a straight line\n", reg::ols_fit(y, x));
}
```

Output

```text
a straight line
                            OLS Regression Results
==============================================================================
Dep. Variable:                         y   R-squared:                         0.999
Model:                               OLS   Adj. R-squared:                   0.9988
Method:                    Least Squares   F-statistic:                        5735
Rank:                                  2   Prob (F-statistic):             3.569e-10
Alpha:                              0.05   Log-Likelihood:                    4.026
No. Observations:                      8   AIC:                              -4.052
Df Residuals:                          6   BIC:                              -3.893
Df Model:                              1   Covariance Type:               nonrobust
==============================================================================
                        coef     std err           t       P>|t|      [0.025      0.975]
------------------------------------------------------------------------------
const                 0.2179      0.1316      1.6552      0.1490     -0.1042      0.5399
x1                    1.9738      0.0261     75.7296  3.5688e-10      1.9100      2.0376
==============================================================================
Omnibus:                            1.16   Durbin-Watson:                      2.86
Prob(Omnibus):                    0.5598   Jarque-Bera (JB):                 0.7909
Skew:                            -0.5117   Prob(JB):                         0.6734
Kurtosis:                          1.849   Cond. No.                          11.48
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
  const reg::OLSFitResult fit;
  report([&] { reg::print_ols_summary(std::cout, fit); });
}
```

Output

```text
subsp::stats::regression::print_ols_summary: fit.coefficients is undefined
```

## See Also

[`ols_summary`](ols_summary.md) returns the text instead of writing it, and [`OLSFitResult`](OLSFitResult.md) holds the numbers.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ols_summary.html">
    <span class="label">Previous</span>
    <span class="title">ols_summary</span>
  </a>
</div>
<!-- apiref:nav:end -->
