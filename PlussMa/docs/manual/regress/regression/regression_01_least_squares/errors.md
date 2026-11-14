<!-- generated from errors.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Errors

[Back to Least Squares](../regression_01_least_squares.md)

`ols_fit` and `ols_predict` throw `pluss::Error` for inputs they cannot work with. The message names the function and the problem, and the same conditions are checked by the other three families under their own function names. The table lists every condition and the program below triggers each one.

| Condition | Message (after the function name) |
|---|---|
| `alpha` not strictly between 0 and 1 | `alpha must be in (0,1)` |
| `rcond` not positive | `rcond must be > 0` |
| `y` and `x` have different numbers of rows | `row mismatch between y and X` |
| `x` has more than two axes | `expected design matrix to be 1D or 2D` |
| no predictor columns and no intercept | `a model needs at least one predictor or an intercept` |
| `predictor_names` has the wrong length | `predictor_names size must equal number of design columns before intercept handling` |
| fewer rows than parameters | `requires n >= number of parameters` |
| a non-finite value with `drop_missing = false` | `non-finite values found and drop_missing=false` |
| no complete row left | `no observations left after filtering missing values` |
| a negative weight, or weights of the wrong length | `weights must be non-negative`, `weights must have one value per row` |
| `ols_predict` with the wrong number of columns | `expected X with either num_predictors columns or k_params columns` |
| `ols_predict` on a default-constructed result | `fit.coefficients is undefined` |

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <limits>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  const double nan = std::numeric_limits<double>::quiet_NaN();
  const std::vector<double> xs{1, 2, 3, 4, 5, 6, 7, 8};
  std::vector<double> ys{2.1, 3.9, 6.2, 7.8, 10.1, 12.2, 13.8, 16.1};
  const auto x = pluss::make_series(xs, {8}, as_f64());
  const auto y = pluss::make_series(ys, {8}, as_f64());
  const auto good = reg::ols_fit(y, x);

  auto attempt = [&](const char *label, auto &&call) {
    try {
      call();
      out.writeln(label, "-> no error");
    } catch (const std::exception &e) {
      const std::string message = e.what();
      out.writeln(label, "->", message.substr(0, message.find('\n')));
    }
  };
  attempt("alpha = 0", [&] { reg::OLSOptions o; o.alpha = 0.0; reg::ols_fit(y, x, o); });
  attempt("alpha = 1", [&] { reg::OLSOptions o; o.alpha = 1.0; reg::ols_fit(y, x, o); });
  attempt("rcond = 0", [&] { reg::OLSOptions o; o.rcond = 0.0; reg::ols_fit(y, x, o); });
  attempt("7 rows against 8", [&] { reg::ols_fit(y, pluss::make_series(std::vector<double>(xs.begin(), xs.begin() + 7), {7}, as_f64())); });
  attempt("three-axis x", [&] { reg::ols_fit(y, pluss::make_series(std::vector<double>(16, 1.0), {8, 2, 1}, as_f64())); });
  attempt("nothing to fit", [&] { reg::OLSOptions o; o.add_intercept = false; reg::ols_fit(y, pluss::series::zeros({8, 0}, as_f64()), o); });
  attempt("two names, one predictor", [&] { reg::OLSOptions o; o.predictor_names = {"a", "b"}; reg::ols_fit(y, x, o); });
  attempt("two rows, three parameters", [&] { reg::ols_fit(pluss::make_series(std::vector<double>{1, 2}, {2}, as_f64()), pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6}, {2, 3}, as_f64())); });
  attempt("NaN with drop_missing off", [&] { ys[0] = nan; reg::OLSOptions o; o.drop_missing = false; reg::ols_fit(pluss::make_series(ys, {8}, as_f64()), x, o); });
  attempt("all responses NaN", [&] { reg::ols_fit(pluss::make_series(std::vector<double>(8, nan), {8}, as_f64()), x); });
  attempt("predict, one column too many", [&] { reg::ols_predict(good, pluss::make_series(std::vector<double>(8, 1.0), {2, 4}, as_f64())); });
  attempt("predict, empty result", [&] { reg::ols_predict(reg::OLSFitResult{}, x); });
}
```

Output

```text
alpha = 0 -> subsp::stats::regression::ols_fit: alpha must be in (0,1)
alpha = 1 -> subsp::stats::regression::ols_fit: alpha must be in (0,1)
rcond = 0 -> subsp::stats::regression::ols_fit: rcond must be > 0
7 rows against 8 -> subsp::stats::regression::ols_fit: row mismatch between y and X
three-axis x -> subsp::stats::regression::ols_fit: expected design matrix to be 1D or 2D
nothing to fit -> subsp::stats::regression::ols_fit: a model needs at least one predictor or an intercept
two names, one predictor -> subsp::stats::regression::ols_fit: predictor_names size must equal number of design columns before intercept handling
two rows, three parameters -> subsp::stats::regression::ols_fit: requires n >= number of parameters
NaN with drop_missing off -> subsp::stats::regression::ols_fit: non-finite values found and drop_missing=false
all responses NaN -> subsp::stats::regression::ols_fit: no observations left after filtering missing values
predict, one column too many -> subsp::stats::regression::ols_predict: expected X with either num_predictors columns or k_params columns
predict, empty result -> subsp::stats::regression::ols_predict: fit.coefficients is undefined
```

Each of the twelve conditions in the table is rejected with the message quoted there, preceded by the qualified name of the function that raised it.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="collinear_predictors.html">
    <span class="label">Previous</span>
    <span class="title">Collinear predictors</span>
  </a>
</div>
<!-- apiref:nav:end -->
