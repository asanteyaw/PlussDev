<!-- generated from shapes_and_inputs.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Shapes and inputs

[Back to Least Squares](../regression_01_least_squares.md)

All four regression families (least squares, generalised linear, quantile and polynomial) read their data the same way, so this page applies to every fit in the guide. The response is one value per observation, the predictors are one column per predictor, and a few shorthand shapes are accepted.

| Argument | Accepted shapes | Read as |
|---|---|---|
| `y` | `[n]`, `[n, 1]`, `[1, n]` | a vector of *n* responses |
| `x` | `[n, p]` | *n* observations of *p* predictors |
| `x` | `[n]` | one predictor (a single column) |
| `x` | `[1, p]` with *p* > 1 | **one column** of *p* values, not one observation of *p* predictors (but see below for prediction) |
| `x` | `[n, 0]` | no predictors: only the intercept is fitted |

In a **prediction**, a fit with two or more predictors reads a `[1, p]` row (or a `[1, k]` row that includes the leading 1) as one new observation, since a single observation is the usual case there; a fit with one predictor keeps the rule in the table, so `[1, 2]` is two values of that predictor.

Anything else is an error: a `y` that is not a vector, an `x` with more than two axes, or an `x` whose number of rows differs from the length of `y`.

## The shorthand shapes

The program fits the same eight points through every accepted shape and prints the coefficients, the number of observations and the number of predictors. The last row has no predictors, so the single coefficient is the sample mean of `y`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const std::vector<double> xs{1, 2, 3, 4, 5, 6, 7, 8};
  const std::vector<double> ys{2.1, 3.9, 6.2, 7.8, 10.1, 12.2, 13.8, 16.1};
  const auto y_vector = pluss::make_series(ys, {8}, as_f64());
  const auto y_column = pluss::make_series(ys, {8, 1}, as_f64());
  const auto y_row = pluss::make_series(ys, {1, 8}, as_f64());
  const auto x_vector = pluss::make_series(xs, {8}, as_f64());
  const auto x_column = pluss::make_series(xs, {8, 1}, as_f64());
  const auto x_row = pluss::make_series(xs, {1, 8}, as_f64());

  auto show = [&](const char *label, const pluss::Series &y, const pluss::Series &x) {
    const auto fit = reg::ols_fit(y, x);
    out.writeln(label, fit.coefficients.pull<double>(), " nobs", fit.nobs, " predictors", fit.num_predictors);
  };
  show("y [8]    x [8]    ", y_vector, x_vector);
  show("y [8, 1] x [8, 1] ", y_column, x_column);
  show("y [1, 8] x [8, 1] ", y_row, x_column);
  show("y [8]    x [1, 8] ", y_vector, x_row);
  show("y [8]    x [8, 0] ", y_vector, pluss::series::zeros({8, 0}, as_f64()));
  out.writeln("mean of y          ", y_vector.mean().item<double>());
}
```

Output

```text
y [8]    x [8]     [0.0357, 1.9976]  nobs 8  predictors 1
y [8, 1] x [8, 1]  [0.0357, 1.9976]  nobs 8  predictors 1
y [1, 8] x [8, 1]  [0.0357, 1.9976]  nobs 8  predictors 1
y [8]    x [1, 8]  [0.0357, 1.9976]  nobs 8  predictors 1
y [8]    x [8, 0]  [9.0250]  nobs 8  predictors 0
mean of y           9.0250
```

The first four rows are one fit: intercept 0.0357 and slope 1.9976. That includes the row vector `x [1, 8]`, which is read as a column of eight values and not as one observation of eight predictors. The last row has no predictor columns, so its single coefficient, 9.0250, is the mean of `y`, and `num_predictors` is 0.

## Other dtypes

The inputs may be any numeric dtype. They are converted to `Float64` before fitting, and every result is `Float64`, so a `Float32` or integer data set is not fitted at single precision.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const std::vector<double> xs{1, 2, 3, 4, 5, 6, 7, 8};
  const std::vector<double> ys{2.1, 3.9, 6.2, 7.8, 10.1, 12.2, 13.8, 16.1};

  const auto double_fit = reg::ols_fit(pluss::make_series(ys, {8}, as_f64()), pluss::make_series(xs, {8}, as_f64()));
  const auto single_fit = reg::ols_fit(pluss::make_series(ys, {8}, as_f32()), pluss::make_series(xs, {8}, as_f32()));
  out.writeln("Float64 input coefficients", double_fit.coefficients.pull<double>());
  out.writeln("Float32 input coefficients", single_fit.coefficients.pull<double>(), " result dtype", single_fit.coefficients.dtype());

  // Integer responses are converted the same way.
  const auto counts = pluss::make_series(std::vector<std::int64_t>{1, 2, 3, 4, 5, 6, 7, 9}, {8}, pluss::Config{}.with_dtype(pluss::ScalarType::Int64));
  const auto int_fit = reg::ols_fit(counts, pluss::make_series(xs, {8}, as_f64()));
  out.writeln("Int64 response coefficients", int_fit.coefficients.pull<double>());
}
```

Output

```text
Float64 input coefficients [0.035714, 1.997619]
Float32 input coefficients [0.035714, 1.997619]  result dtype float64
Int64 response coefficients [-0.250000, 1.083333]
```

Float32 input gives the same coefficients, and the result dtype is `float64`: the data were converted before fitting, so a single-precision data set is not fitted at single precision. The integer response in the last row is a different data set (1, 2, ..., 7, 9), so its coefficients differ; the point is that it is accepted.

## Names

`response_name` and `predictor_names` label the output. The coefficient list always starts with `const` when an intercept is fitted, and the predictors are named `x1`, `x2`, ... unless names are given. The number of names must equal the number of predictor columns, not counting the intercept.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  const auto y = pluss::make_series(std::vector<double>{3.1, 4.9, 7.2, 8.8, 11.1, 12.9}, {6}, as_f64());
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 2, 1, 4, 3, 6, 5}, {6, 2}, as_f64());

  const auto plain = reg::ols_fit(y, x);
  out.writeln("default names             ", plain.coefficient_names, " response", plain.response_name);

  reg::OLSOptions named;
  named.response_name = "price";
  named.predictor_names = {"area", "rooms"};
  const auto labelled = reg::ols_fit(y, x, named);
  out.writeln("with names                ", labelled.coefficient_names, " response", labelled.response_name);

  reg::OLSOptions through_origin = named;
  through_origin.add_intercept = false;
  const auto origin = reg::ols_fit(y, x, through_origin);
  out.writeln("no intercept              ", origin.coefficient_names);
  out.writeln("k_params / num_predictors ", labelled.k_params, "/", labelled.num_predictors, " and ", origin.k_params, "/", origin.num_predictors);
}
```

Output

```text
default names              [const, x1, x2]  response y
with names                 [const, area, rooms]  response price
no intercept               [area, rooms]
k_params / num_predictors  3 / 2  and  2 / 2
```

Without names the coefficients are `const`, `x1`, `x2` and the response is `y`. With names they are `const`, `area`, `rooms` and the response is `price`. Setting `add_intercept = false` removes `const`: `k_params` falls from 3 to 2 while `num_predictors` stays at 2.

## Inputs that are rejected

Every malformed input stops the fit with a `pluss::Error` that names the function and the problem. The program prints the first line of each message (the error also records where it was raised).

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  const std::vector<double> xs{1, 2, 3, 4, 5, 6, 7, 8};
  const std::vector<double> ys{2.1, 3.9, 6.2, 7.8, 10.1, 12.2, 13.8, 16.1};
  const auto y = pluss::make_series(ys, {8}, as_f64());
  const auto x = pluss::make_series(xs, {8}, as_f64());

  auto attempt = [&](const char *label, auto &&fit) {
    try {
      fit();
      out.writeln(label, "-> fitted");
    } catch (const std::exception &e) {
      const std::string message = e.what();
      out.writeln(label, "->", message.substr(0, message.find('\n')));
    }
  };
  attempt("x has 7 rows for 8 responses  ", [&] { reg::ols_fit(y, pluss::make_series(std::vector<double>(xs.begin(), xs.begin() + 7), {7}, as_f64())); });
  attempt("x has three axes              ", [&] { reg::ols_fit(y, pluss::make_series(std::vector<double>(16, 1.0), {8, 2, 1}, as_f64())); });
  attempt("three names for two predictors", [&] {
    reg::OLSOptions o;
    o.predictor_names = {"a", "b", "c"};
    reg::ols_fit(y, pluss::make_series(std::vector<double>(16, 1.0), {8, 2}, as_f64()), o);
  });
  attempt("no predictors and no intercept", [&] {
    reg::OLSOptions o;
    o.add_intercept = false;
    reg::ols_fit(y, pluss::series::zeros({8, 0}, as_f64()), o);
  });
  attempt("more parameters than rows     ", [&] {
    reg::ols_fit(pluss::make_series(std::vector<double>{1, 2}, {2}, as_f64()), pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6}, {2, 3}, as_f64()));
  });
}
```

Output

```text
x has 7 rows for 8 responses   -> subsp::stats::regression::ols_fit: row mismatch between y and X
x has three axes               -> subsp::stats::regression::ols_fit: expected design matrix to be 1D or 2D
three names for two predictors -> subsp::stats::regression::ols_fit: predictor_names size must equal number of design columns before intercept handling
no predictors and no intercept -> subsp::stats::regression::ols_fit: a model needs at least one predictor or an intercept
more parameters than rows      -> subsp::stats::regression::ols_fit: requires n >= number of parameters
```

Each message begins with the qualified name of the function that raised it and then states the problem, so a failure inside a larger program points at the call that caused it.
