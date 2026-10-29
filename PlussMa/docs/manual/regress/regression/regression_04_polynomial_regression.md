<!-- generated from regression_04_polynomial_regression.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Regression 4: Polynomial Regression

A polynomial regression fits a curve: each predictor is replaced by its powers (and optionally their products), and the model is fitted by least squares on the expanded columns. It is therefore an ordinary least-squares fit of a nonlinear curve, with the same standard errors, intervals and diagnostics. After this chapter you can fit a curve, read the layout of the expanded design, choose the degree, avoid the ill conditioning that high degrees bring, and predict.

Every program is complete, and its output is the output of that program.

## The calls

```cpp
reg::PolyFitResult reg::poly_fit(const Series &y, const Series &x, const reg::PolyOptions &options = {}, Config cfg = {});
Series             reg::poly_expand(const Series &x, std::int64_t degree, bool include_cross = false, Config cfg = {});
Series             reg::poly_predict(const reg::PolyFitResult &fit, const Series &x_new, Config cfg = {});
std::string        reg::poly_summary(const reg::PolyFitResult &fit);
std::ostream      &reg::print_poly_summary(std::ostream &os, const reg::PolyFitResult &fit);
```

Inputs are read as for least squares ([Shapes and inputs](regression_01_least_squares/shapes_and_inputs.md)).

| `PolyOptions` | Meaning | Default |
|---|---|---|
| `degree` | the highest power; at least 1 | 2 |
| `include_cross` | add products of different predictors with total degree at most `degree` | false |
| `weights` | precision weights, as for least squares | none |
| `add_intercept`, `drop_missing`, `rcond`, `alpha` | as for least squares | true, true, 1e-12, 0.05 |
| `response_name`, `predictor_names` | labels; one name per column of `x` | `"y"`, none |

| `PolyFitResult` | Meaning |
|---|---|
| `ols` | the complete least-squares fit of the expanded design, an `OLSFitResult` ([Least Squares](regression_01_least_squares.md)); `ols.coefficient_names` are the term names |
| `degree`, `include_cross` | the settings used |
| `base_predictor_names` | the names of the original predictors |

## Programs

- [Fitting a curve](regression_04_polynomial_regression/fitting_a_curve.md)
- [The expanded design](regression_04_polynomial_regression/the_expanded_design.md)
- [Choosing the degree](regression_04_polynomial_regression/choosing_the_degree.md)
- [Prediction, the summary and options](regression_04_polynomial_regression/prediction_summary_and_options.md)

## Checklist

1. R² cannot choose the degree: it never decreases. Use `aic` and `bic`, and error on data the model has not seen.
2. Read `rank` and `condition_number`. Centre and scale a predictor that is far from zero or has a narrow range; the fitted curve does not change.
3. Prefer low degrees. A high-degree polynomial is most unreliable outside the range of the data, exactly where predictions are most tempting.
4. Cross terms multiply the number of columns; make sure there are many more rows than terms.
5. `poly_predict` takes the original predictors, with the same number of columns as the fit.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

regression_04_polynomial_regression/fitting_a_curve
regression_04_polynomial_regression/the_expanded_design
regression_04_polynomial_regression/choosing_the_degree
regression_04_polynomial_regression/prediction_summary_and_options
```
<!-- apiref:toctree:end -->
