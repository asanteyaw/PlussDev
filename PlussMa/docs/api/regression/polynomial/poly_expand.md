<!-- generated from poly_expand.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `regression::poly_expand`

Builds the matrix of polynomial terms of one or more predictors: each predictor raised to the powers one to the degree, and optionally the products of different predictors.

## Signature

```cpp
Series regression::poly_expand(const Series &x, std::int64_t degree, bool include_cross = false, Config cfg = {});
```

## What It Does

For a predictor `x`, returns the columns x, x², ..., x^degree. With several predictors, each predictor's powers are grouped together in the order of the columns of `x`: x1, x1², ..., x2, x2², ... With `include_cross` the mixed monomials of two or more predictors up to total degree `degree` follow, such as x1·x2 for degree two and x1²·x2 and x1·x2² as well for degree three. No constant column is added: that is `poly_fit`'s job, and `poly_fit` calls this function to build its design. Use it directly to expand features for a model that is not least squares.

## Parameters

`x`
: The predictors, `[n]` for one or `[n, p]` for `p`.

`degree`
: The highest power, at least one.

`include_cross`
: Whether to append the mixed terms. It has no effect with one predictor.

`cfg`
: Computation configuration for the returned `Series`.

## Returns

A `Series` of shape `[n, columns]`, with p × degree columns plus the mixed terms when `include_cross` is true.

## Errors

Throws `poly_expand: degree must be >= 1` for a degree below one.

## Practical Examples

### Example 1: one predictor

A cubic expansion of three values. Each row is x, x² and x³.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(0));
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3}, {3}, as_f64());
  const auto z = reg::poly_expand(x, 3);
  out.writeln("shape", z.sizes());
  const std::vector<std::vector<double>> rows = z.host<double>();
  for (const auto &row : rows) out.writeln(row);
}
```

Output

```text
shape [3, 3]
[1, 1, 1]
[2, 4, 8]
[3, 9, 27]
```

### Example 2: two predictors

Four rows of two predictors, quadratic, without cross terms: the columns are x1, x1², x2, x2².

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(0));
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 2, 1, 4, 3}, {2, 4}, as_f64()).transpose(0, 1);
  const auto z = reg::poly_expand(x, 2);
  out.writeln("shape", z.sizes());
  const std::vector<std::vector<double>> rows = z.host<double>();
  for (const auto &row : rows) out.writeln(row);
}
```

Output

```text
shape [4, 4]
[1, 1, 2, 4]
[2, 4, 1, 1]
[3, 9, 4, 16]
[4, 16, 3, 9]
```

### Example 3: with cross terms

The same rows with the product x1·x2 appended as the last column.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(0));
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 2, 1, 4, 3}, {2, 4}, as_f64()).transpose(0, 1);
  const auto z = reg::poly_expand(x, 2, true);
  out.writeln("shape", z.sizes());
  const std::vector<std::vector<double>> rows = z.host<double>();
  for (const auto &row : rows) out.writeln(row);
}
```

Output

```text
shape [4, 5]
[1, 1, 2, 4, 2]
[2, 4, 1, 1, 2]
[3, 9, 4, 16, 12]
[4, 16, 3, 9, 12]
```

### Example 4: how many columns

For two predictors the quadratic has four columns plain and five with the cross term; the cubic has six plain and nine with all mixed terms up to total degree three.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(0));
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 2, 1, 4, 3}, {2, 4}, as_f64()).transpose(0, 1);
  for (const std::int64_t degree : {2, 3}) {
    out.writeln("degree", degree, " plain", reg::poly_expand(x, degree).size(1), " with cross", reg::poly_expand(x, degree, true).size(1));
  }
}
```

Output

```text
degree 2  plain 4  with cross 5
degree 3  plain 6  with cross 9
```

### Example 5: a degree below one

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(0));
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3}, {3}, as_f64());
  try {
    reg::poly_expand(x, 0);
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find("Triggered")));
  }
}
```

Output

```text
subsp::stats::regression::poly_expand: degree must be >= 1
```

### Example 6: the expansion as features for another model

`poly_expand` makes polynomial features for any model, not only least squares. The program fits the 0.9 quantile of the curved data with the expanded columns, a quantile regression of a quadratic.

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
  reg::QuantileOptions options;
  options.tau = 0.9;
  options.predictor_names = {"x", "x^2"};
  const auto fit = reg::quantile_fit(y, reg::poly_expand(x, 2), options);
  out.writeln(fit.coefficient_names, fit.coefficients.items<double>());
}
```

Output

```text
[const, x, x^2] [2.77500, -0.41667, 0.74167]
```

### Example 7: a Poisson model of a curved rate

The expansion also feeds `glm_fit`. The program fits counts whose log-mean is quadratic in the dose by expanding the dose and using the Poisson family.

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
  options.predictor_names = {"dose", "dose^2"};
  const auto fit = reg::glm_fit(y, reg::poly_expand(x, 2), options);
  out.writeln(fit.coefficient_names, fit.coefficients.items<double>());
  out.writeln("deviance", fit.deviance, " iterations", fit.iterations, " converged", fit.converged);
}
```

Output

```text
[const, dose, dose^2] [-0.63184, 0.69208, -0.00932]
deviance 3.84830  iterations 7  converged true
```

## See Also

[`poly_fit`](poly_fit.md) fits a least-squares model on this expansion, and [`PolyOptions`](PolyOptions.md) carries the degree and cross-term choices.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="PolyFitResult.html">
    <span class="label">Previous</span>
    <span class="title">PolyFitResult</span>
  </a>
  <a class="next-link" href="poly_fit.html">
    <span class="label">Next</span>
    <span class="title">poly_fit</span>
  </a>
</div>
<!-- apiref:nav:end -->
