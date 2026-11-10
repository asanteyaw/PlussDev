<!-- generated from poly_summary.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `regression::poly_summary`

Returns the report of a fitted polynomial regression as text.

## Signature

```cpp
std::string regression::poly_summary(const regression::PolyFitResult &fit);
```

## What It Does

Formats a first line that names the degree and whether cross terms were included, `Polynomial Regression (degree=2, cross=false)`, followed by the least-squares report of the nested fit exactly as `ols_summary` produces it, with the expanded columns named `x1`, `x1^2`, and so on. The model line of the report therefore still reads `OLS`: a polynomial fit is a least-squares fit of the expanded design. The text is what `print_poly_summary` writes to a stream; use this function when you want to keep or post-process it.

## Parameters

`fit`
: A completed result from `poly_fit`.

## Returns

A `std::string` holding the report, with line breaks.

## Errors

Throws `subsp::stats::regression::print_poly_summary: fit.ols.coefficients is undefined` for a default-constructed result, which has nothing to report.

## Practical Examples

### Example 1: a quadratic report

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
  options.response_name = "load";
  options.predictor_names = {"t"};
  out.writeln(reg::poly_summary(reg::poly_fit(y, x, options)));
}
```

Output

```text
Polynomial Regression (degree=2, cross=false)
                            OLS Regression Results
==============================================================================
Dep. Variable:                      load   R-squared:                             1
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
t                    -0.5298      0.0478    -11.0732  1.0464e-04     -0.6527     -0.4068
t^2                   0.7536      0.0052    145.2202  2.9373e-10      0.7402      0.7669
==============================================================================
Omnibus:                          0.8929   Durbin-Watson:                     2.596
Prob(Omnibus):                    0.6399   Jarque-Bera (JB):                 0.6508
Skew:                             0.3757   Prob(JB):                         0.7222
Kurtosis:                          1.822   Cond. No.                          146.5
==============================================================================
```

### Example 2: with a cross term

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x1 = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, {10}, as_f64());
  const auto x2 = pluss::make_series(std::vector<double>{3, 1, 4, 1, 5, 9, 2, 6, 5, 3}, {10}, as_f64());
  const auto xx = x1.unsqueeze(1).unite({x2.unsqueeze(1)}, 1);
  const auto yy = pluss::make_series(std::vector<double>{4.22, 3.7, 8.45, 6.6, 13.35, 22.38, 11.7, 21.82, 21.45, 18.6}, {10}, as_f64());
  reg::PolyOptions options;
  options.include_cross = true;
  options.predictor_names = {"a", "b"};
  out.writeln(reg::poly_summary(reg::poly_fit(yy, xx, options)));
}
```

Output

```text
Polynomial Regression (degree=2, cross=true)
                            OLS Regression Results
==============================================================================
Dep. Variable:                         y   R-squared:                        0.9995
Model:                               OLS   Adj. R-squared:                    0.999
Method:                    Least Squares   F-statistic:                        1764
Rank:                                  6   Prob (F-statistic):             8.982e-07
Alpha:                              0.05   Log-Likelihood:                    4.796
No. Observations:                     10   AIC:                               2.408
Df Residuals:                          4   BIC:                               4.224
Df Model:                              5   Covariance Type:               nonrobust
==============================================================================
                        coef     std err           t       P>|t|      [0.025      0.975]
------------------------------------------------------------------------------
const                 1.0950      0.4442      2.4653      0.0693     -0.1382      2.3283
a                     0.9760      0.1390      7.0203      0.0022      0.5900      1.3620
a^2                   0.0027      0.0119      0.2276      0.8311     -0.0304      0.0359
b                     0.4752      0.1500      3.1670      0.0340      0.0586      0.8918
b^2                   0.0037      0.0137      0.2686      0.8015     -0.0343      0.0416
a*b                   0.1996      0.0228      8.7663  9.3351e-04      0.1364      0.2628
==============================================================================
Omnibus:                          0.2752   Durbin-Watson:                     2.828
Prob(Omnibus):                    0.8714   Jarque-Bera (JB):                 0.3842
Skew:                             0.2817   Prob(JB):                         0.8252
Kurtosis:                          2.222   Cond. No.                          390.3
==============================================================================
```

### Example 3: the first line and then the least-squares report

The text is a one-line heading and then the report of the nested fit: dropping the first line leaves exactly `ols_summary` of `fit.ols`.

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
  const auto text = reg::poly_summary(fit);
  const auto newline = text.find('\n');
  out.writeln("first line:", text.substr(0, newline));
  out.writeln("the rest is ols_summary of fit.ols:", text.substr(newline + 1) == reg::ols_summary(fit.ols));
}
```

Output

```text
first line: Polynomial Regression (degree=2, cross=false)
the rest is ols_summary of fit.ols: true
```

## See Also

[`print_poly_summary`](print_poly_summary.md) writes the same text to a stream, and [`PolyFitResult`](PolyFitResult.md) holds the numbers.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="poly_predict.html">
    <span class="label">Previous</span>
    <span class="title">poly_predict</span>
  </a>
  <a class="next-link" href="print_poly_summary.html">
    <span class="label">Next</span>
    <span class="title">print_poly_summary</span>
  </a>
</div>
<!-- apiref:nav:end -->
