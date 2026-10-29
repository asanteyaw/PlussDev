<!-- generated from regression_05_worked_problems.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Regression 5: Worked Problems

Five problems that combine the families of the guide with the quantities in their results. Each states the question, answers it with programs whose every step is checked, and shows where the first answer that comes to mind goes wrong.

- [A calibration line and its uncertainty](regression_05_worked_problems/a_calibration_line_and_its_uncertainty.md) uses `ols_fit`, its covariance matrix and a *t* distribution to test a claimed sensitivity and to invert the line, with an interval from the delta method checked against the inversion of the prediction band.
- [Dose-response and the median effective dose](regression_05_worked_problems/dose_response_and_the_median_effective_dose.md) uses `glm_fit` with the binomial family to estimate the dose at which half of the subjects respond, with a delta-method interval and Fieller's interval, and shows when they disagree.
- [Comparing two event rates](regression_05_worked_problems/comparing_two_event_rates.md) uses a Poisson regression on a group indicator, with a rate ratio, three tests of no difference, and a check on the dispersion.
- [Reference curves with quantiles](regression_05_worked_problems/reference_curves_with_quantiles.md) combines `poly_expand` and `quantile_fit` to draw percentile curves for a measurement whose spread grows with age, and places new observations on them.
- [The same data, four ways](regression_05_worked_problems/the_same_data_four_ways.md) fits one data set with a straight line, a quadratic, the normal family with a log link and a median line, and scores them against a known truth, inside and beyond the data, with and without an outlier.

## Checklist

1. A quantity built from several coefficients (a ratio, a sum, an inverse prediction) needs the `covariance` matrix, not just the standard errors.
2. A ratio with an uncertain denominator is better described by Fieller's interval than by a symmetric one.
3. Test a hypothesis about a coefficient with the value in the hypothesis, `(b − b₀) / se`, and the right reference distribution: Student's *t* for least squares and quantile regression, the normal distribution for a GLM.
4. Check the dispersion before trusting a Poisson test.
5. Choose the family for what is wrong with the data, and the form of the model for how the response behaves: an outlier calls for a quantile fit, curvature for a polynomial or a link, a bounded or count response for a GLM.
6. Predictions beyond the range of the data depend entirely on the form of the model.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

regression_05_worked_problems/a_calibration_line_and_its_uncertainty
regression_05_worked_problems/dose_response_and_the_median_effective_dose
regression_05_worked_problems/comparing_two_event_rates
regression_05_worked_problems/reference_curves_with_quantiles
regression_05_worked_problems/the_same_data_four_ways
```
<!-- apiref:toctree:end -->
