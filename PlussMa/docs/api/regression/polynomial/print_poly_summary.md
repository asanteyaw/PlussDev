<!-- generated from print_poly_summary.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `regression::print_poly_summary`

Writes the report of a fitted polynomial regression to an output stream.

## Signature

```cpp
std::ostream &regression::print_poly_summary(std::ostream &os, const regression::PolyFitResult &fit);
```

## What It Does

Writes exactly the text that `poly_summary` returns to `os` and returns `os`, so the call can be chained or used with any stream: `std::cout`, a file, a string stream.

## Parameters

`os`
: The destination stream.

`fit`
: A completed result from `poly_fit`.

## Returns

The stream `os`.

## Errors

Throws `subsp::stats::regression::print_poly_summary: fit.ols.coefficients is undefined` for a default-constructed result, which has nothing to report.

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
  const auto y = pluss::make_series(std::vector<double>{3.1, 4.9, 8.2, 12.8, 19.1, 26.8, 36.2, 46.9}, {8}, as_f64());
  reg::print_poly_summary(std::cout, reg::poly_fit(y, x));
}
```

Output

```text
Polynomial Regression (degree=2, cross=false)
                            OLS Regression Results
==============================================================================
Dep. Variable:                         y   R-squared:                             1
Model:                               OLS   Adj. R-squared:                        1
Method:                    Least Squares   F-statistic:                    1.92e+05
Rank:                                  3   Prob (F-statistic):             6.116e-13
Alpha:                              0.05   Log-Likelihood:                    12.12
No. Observations:                      8   AIC:                              -18.24
Df Residuals:                          5   BIC:                              -18.01
Df Model:                              2   Covariance Type:               nonrobust
==============================================================================
                        coef     std err           t       P>|t|      [0.025      0.975]
------------------------------------------------------------------------------
const                 2.9179      0.0938     31.0951  6.4571e-07      2.6766      3.1591
x1                   -0.5298      0.0478    -11.0732  1.0464e-04     -0.6527     -0.4068
x1^2                  0.7536      0.0052    145.2202  2.9373e-10      0.7402      0.7669
==============================================================================
Omnibus:                          0.8929   Durbin-Watson:                     2.596
Prob(Omnibus):                    0.6399   Jarque-Bera (JB):                 0.6508
Skew:                             0.3757   Prob(JB):                         0.7222
Kurtosis:                          1.822   Cond. No.                          146.5
==============================================================================
```

### Example 2: into a string stream

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(0));
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8}, {8}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{3.1, 4.9, 8.2, 12.8, 19.1, 26.8, 36.2, 46.9}, {8}, as_f64());
  const auto fit = reg::poly_fit(y, x);
  std::ostringstream stream;
  reg::print_poly_summary(stream, fit);
  out.writeln("same as poly_summary:", stream.str() == reg::poly_summary(fit));
}
```

Output

```text
same as poly_summary: true
```

### Example 3: chaining

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8}, {8}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{3.1, 4.9, 8.2, 12.8, 19.1, 26.8, 36.2, 46.9}, {8}, as_f64());
  reg::PolyOptions options;
  options.degree = 3;
  reg::print_poly_summary(std::cout << "a cubic\n", reg::poly_fit(y, x, options));
}
```

Output

```text
a cubic
Polynomial Regression (degree=3, cross=false)
                            OLS Regression Results
==============================================================================
Dep. Variable:                         y   R-squared:                             1
Model:                               OLS   Adj. R-squared:                        1
Method:                    Least Squares   F-statistic:                    1.23e+05
Rank:                                  4   Prob (F-statistic):             2.203e-10
Alpha:                              0.05   Log-Likelihood:                    12.86
No. Observations:                      8   AIC:                              -17.71
Df Residuals:                          4   BIC:                              -17.39
Df Model:                              3   Covariance Type:               nonrobust
==============================================================================
                        coef     std err           t       P>|t|      [0.025      0.975]
------------------------------------------------------------------------------
const                 2.7929      0.1691     16.5194  7.8639e-05      2.3235      3.2623
x1                   -0.3997      0.1530     -2.6129      0.0592     -0.8244      0.0250
x1^2                  0.7195      0.0384     18.7498  4.7640e-05      0.6129      0.8260
x1^3                  0.0025      0.0028      0.8970      0.4204     -0.0053      0.0103
==============================================================================
Omnibus:                          0.6892   Durbin-Watson:                     3.047
Prob(Omnibus):                    0.7085   Jarque-Bera (JB):                 0.5874
Skew:                             0.4685   Prob(JB):                         0.7455
Kurtosis:                           2.06   Cond. No.                           2242
==============================================================================
Warnings:
[1] The condition number is large, 2242. This might indicate strong multicollinearity or other numerical problems.
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
  const reg::PolyFitResult fit;
  report([&] { reg::print_poly_summary(std::cout, fit); });
}
```

Output

```text
subsp::stats::regression::print_poly_summary: fit.ols.coefficients is undefined
```

## See Also

[`poly_summary`](poly_summary.md) returns the text instead of writing it, and [`PolyFitResult`](PolyFitResult.md) holds the numbers.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="poly_summary.html">
    <span class="label">Previous</span>
    <span class="title">poly_summary</span>
  </a>
</div>
<!-- apiref:nav:end -->
