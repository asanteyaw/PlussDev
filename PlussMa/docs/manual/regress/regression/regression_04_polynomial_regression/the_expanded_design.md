<!-- generated from the_expanded_design.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The expanded design

[Back to Polynomial Regression](../regression_04_polynomial_regression.md)

`poly_expand` returns the design matrix that `poly_fit` regresses on, so the layout is visible. It does **not** include the column of ones; the intercept is added by the fit, as for any other least-squares model.

For *p* predictors and the maximum power *d*:

- Without cross terms (`include_cross = false`, the default) the columns are the powers of each predictor in turn: x₁, x₁², ..., x₁ᵈ, then x₂, x₂², ..., x₂ᵈ, and so on. There are *p·d* of them.
- With cross terms (`include_cross = true`) all of those come first, followed by every product of different predictors whose total degree is at most *d*, for example x₁x₂ and x₁²x₂. There are C(*p* + *d*, *d*) − 1 columns in all, one for every monomial of degree 1 to *d*.
- Within a total degree the cross terms are ordered by the exponent of the first predictor, then of the second, and so on, in increasing order.
- Cross terms need at least two predictors and a degree of at least 2; otherwise the option has no effect.

A one-row `x` of shape `[1, p]` is read as a single column, as everywhere else ([Shapes and inputs](../regression_01_least_squares/shapes_and_inputs.md)). A `degree` below 1 is an error.

## One and two predictors

The program prints the expansion of small data sets, with the shapes, so the order of the columns can be read off.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(0));
  const auto one = pluss::make_series(std::vector<double>{1, 2, 3}, {3}, as_f64());
  const auto powers = reg::poly_expand(one, 3);
  out.writeln("x = 1, 2, 3 at degree 3: shape", powers.sizes());
  out.writeln("   rows", powers.host<double>().array().data);

  // Two predictors; the first row is (x1, x2) = (1, 2) and the second (3, 4).
  const auto two = pluss::make_series(std::vector<double>{1, 2, 3, 4}, {2, 2}, as_f64());
  const auto plain = reg::poly_expand(two, 3);
  const auto crossed = reg::poly_expand(two, 3, true);
  out.writeln("two predictors, degree 3, no cross terms: shape", plain.sizes());
  out.writeln("   columns x1 x1^2 x1^3 x2 x2^2 x2^3; rows", plain.host<double>().array().data);
  out.writeln("two predictors, degree 3, cross terms: shape", crossed.sizes());
  out.writeln("   the first six columns are the same:", plain.sub(crossed.index_select(1, pluss::Series::arange(0, 6, 1, pluss::ScalarType::Int64))).abs().max().item<double>());
  out.writeln("   rows", crossed.host<double>().array().data);

  // A row vector is one column.
  out.writeln("a [1, 3] input at degree 2:", reg::poly_expand(pluss::make_series(std::vector<double>{1, 2, 3}, {1, 3}, as_f64()), 2).sizes());
  out.writeln("cross terms have no effect at degree 1:", reg::poly_expand(two, 1, true).sizes(), " or with one predictor:", reg::poly_expand(one, 3, true).sizes());
}
```

Output

```text
x = 1, 2, 3 at degree 3: shape [3, 3]
   rows [1, 1, 1, 2, 4, 8, 3, 9, 27]
two predictors, degree 3, no cross terms: shape [2, 6]
   columns x1 x1^2 x1^3 x2 x2^2 x2^3; rows [1, 1, 1, 2, 4, 8, 3, 9, 27, 4, 16, 64]
two predictors, degree 3, cross terms: shape [2, 9]
   the first six columns are the same: 0
   rows [1, 1, 1, 2, 4, 8, 2, 4, 2, 3, 9, 27, 4, 16, 64, 12, 48, 36]
a [1, 3] input at degree 2: [3, 2]
cross terms have no effect at degree 1: [2, 2]  or with one predictor: [3, 3]
```

The columns of one predictor are the powers 1, 2 and 3 of each value, and for two predictors the columns are x₁, x₁², x₁³, x₂, x₂², x₂³. With cross terms the first six columns are the same and three more follow: for the first row (x₁, x₂) = (1, 2) they are x₁x₂ = 2, x₁x₂² = 4 and x₁²x₂ = 2. A `[1, 3]` input is one column of three values, and cross terms change nothing at degree 1 or with a single predictor. Within a total degree the cross terms are ordered by the exponent of the first predictor, then of the second, in increasing order.

## Names and the number of columns

`poly_fit` names the columns. A power is written `x1^2`, a product `x1*x2`, and a mixed term `x1^2*x2`. The program fits three predictors at degree 3 with cross terms, prints every name in column order, and checks the column count against C(*p* + *d*, *d*) − 1.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const std::int64_t n = 40;
  std::vector<double> flat, ys;
  for (int i = 0; i < n; ++i) {
    const double a = (i % 8) * 0.5, b = ((i * 3) % 7) * 0.4, c = ((i * 5) % 11) * 0.3;
    flat.push_back(a);
    flat.push_back(b);
    flat.push_back(c);
    ys.push_back(1.0 + a - 0.5 * b + 0.2 * c + 0.3 * a * b + 0.05 * std::fmod(i * 0.6180339887498949, 1.0));
  }
  reg::PolyOptions options;
  options.degree = 3;
  options.include_cross = true;
  options.predictor_names = {"a", "b", "c"};
  const auto fit = reg::poly_fit(pluss::make_series(ys, {n}, as_f64()), pluss::make_series(flat, {n, 3}, as_f64()), options);

  out.writeln("coefficient names:", fit.ols.coefficient_names);
  out.writeln("k_params", fit.ols.k_params, " num_predictors (expanded columns)", fit.ols.num_predictors, " rank", fit.ols.rank);

  auto choose = [](int top, int bottom) {
    double value = 1.0;
    for (int i = 1; i <= bottom; ++i) {
      value = value * (top - bottom + i) / i;
    }
    return static_cast<std::int64_t>(value + 0.5);
  };
  out.writeln("C(p + d, d) - 1 for p = 3, d = 3:", choose(6, 3) - 1, " and for p = 2, d = 3:", choose(5, 3) - 1, " p = 4, d = 2:", choose(6, 2) - 1);
  out.writeln("base predictor names", fit.base_predictor_names);
}
```

Output

```text
coefficient names: [const, a, a^2, a^3, b, b^2, b^3, c, c^2, c^3, b*c, a*c, a*b, b*c^2, b^2*c, a*c^2, a*b*c, a*b^2, a^2*c, a^2*b]
k_params 20  num_predictors (expanded columns) 19  rank 20
C(p + d, d) - 1 for p = 3, d = 3: 19  and for p = 2, d = 3: 9  p = 4, d = 2: 14
base predictor names [a, b, c]
```

The twenty parameters are the constant and 19 expanded columns, in the order of the printed names: the powers of a, then of b, then of c, and then the cross terms, ordered by total degree (the degree-2 products b*c, a*c and a*b first, then those of degree 3). The count agrees with C(*p* + *d*, *d*) − 1 for the three settings shown. The number of terms grows quickly with the degree and the number of predictors (14 for four predictors at degree 2, 19 for three at degree 3), so the number of rows must exceed it.
