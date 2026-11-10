<!-- generated from quantile_summary.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `regression::quantile_summary`

Returns the report of a fitted quantile regression as text.

## Signature

```cpp
std::string regression::quantile_summary(const regression::QuantileFitResult &fit);
```

## What It Does

Formats the quantile, observation counts, degrees of freedom, objective, pseudo R², residual density and level, followed by a table of the coefficients with their standard errors, t statistics, p-values and confidence intervals. It is the same text `print_quantile_summary` writes to a stream; use this function when you want to keep or post-process it.

## Parameters

`fit`
: A completed result from `quantile_fit`.

## Returns

A `std::string` holding the report, with line breaks.

## Errors

Throws `subsp::stats::regression::print_quantile_summary: fit.coefficients is undefined` for a default-constructed result, which has nothing to report.

## Practical Examples

### Example 1: the median report

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}, {12}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{2.1, 3.9, 3.2, 6.8, 5.1, 9.7, 6.4, 12.9, 7.7, 16.5, 8.2, 20.1}, {12}, as_f64());
  reg::QuantileOptions options;
  options.response_name = "price";
  options.predictor_names = {"size"};
  out.writeln(reg::quantile_summary(reg::quantile_fit(y, x, options)));
}
```

Output

```text
                         Quantile Regression Results
==============================================================================
Dep. Variable:                     price   Pseudo R-squared:                 0.4101
Model:                          Quantile   Tau:                                 0.5
Method:                             IRLS   No. Iterations:                       21
Rank:                                  2   Converged:                          true
Alpha:                              0.05   Objective:                         14.04
No. Observations:                     12   Df Residuals:                         10
Df Model:                              1   Residual Density(0):                0.0764
==============================================================================
                        coef     std err           t       P>|t|      [0.025      0.975]
------------------------------------------------------------------------------
const                 0.5800      4.0277      0.1440      0.8884     -8.3942      9.5542
size                  1.5200      0.5473      2.7775      0.0195      0.3006      2.7394
==============================================================================
```

### Example 2: the 0.9 quantile report

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}, {12}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{2.1, 3.9, 3.2, 6.8, 5.1, 9.7, 6.4, 12.9, 7.7, 16.5, 8.2, 20.1}, {12}, as_f64());
  reg::QuantileOptions options;
  options.tau = 0.9;
  options.predictor_names = {"size"};
  out.writeln(reg::quantile_summary(reg::quantile_fit(y, x, options)));
}
```

Output

```text
                         Quantile Regression Results
==============================================================================
Dep. Variable:                         y   Pseudo R-squared:                 0.7641
Model:                          Quantile   Tau:                                 0.9
Method:                             IRLS   No. Iterations:                       36
Rank:                                  2   Converged:                          true
Alpha:                              0.05   Objective:                         3.168
No. Observations:                     12   Df Residuals:                         10
Df Model:                              1   Residual Density(0):                0.1522
==============================================================================
                        coef     std err           t       P>|t|      [0.025      0.975]
------------------------------------------------------------------------------
const                 0.6600      1.9956      0.3307      0.7477     -3.7865      5.1065
size                  1.6200      0.2971      5.4533  2.7957e-04      0.9581      2.2819
==============================================================================
```

### Example 3: keeping the text

The summary is an ordinary string, so it can be measured, searched or saved.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(0));
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}, {12}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{2.1, 3.9, 3.2, 6.8, 5.1, 9.7, 6.4, 12.9, 7.7, 16.5, 8.2, 20.1}, {12}, as_f64());
  const auto text = reg::quantile_summary(reg::quantile_fit(y, x));
  out.writeln("characters", text.size(), " mentions the residual density:", text.find("Residual Density") != std::string::npos);
}
```

Output

```text
characters 1252  mentions the residual density: true
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
  const reg::QuantileFitResult fit;
  report([&] { (void)reg::quantile_summary(fit); });
}
```

Output

```text
subsp::stats::regression::print_quantile_summary: fit.coefficients is undefined
```

## See Also

[`print_quantile_summary`](print_quantile_summary.md) writes the same text to a stream, and [`QuantileFitResult`](QuantileFitResult.md) holds every number the report shows.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="quantile_predict.html">
    <span class="label">Previous</span>
    <span class="title">quantile_predict</span>
  </a>
  <a class="next-link" href="print_quantile_summary.html">
    <span class="label">Next</span>
    <span class="title">print_quantile_summary</span>
  </a>
</div>
<!-- apiref:nav:end -->
