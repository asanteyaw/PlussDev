<!-- generated from ts_06_direct_forecasting.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Time Series 6: Direct Forecasting and Model Comparison

The ARIMA, VAR and state-space models of the earlier chapters forecast several steps ahead by feeding their own forecasts back in. A *direct* forecaster takes a different route: for each horizon `h` it fits a separate regression that predicts `y_{t+h}` straight from lagged information available at time `t`. It needs no model of how the predictors themselves evolve, it can use exogenous series whose future values are known (a calendar, a planned price, a weather forecast), and any regression learner can be used. After this chapter you can split a series for honest evaluation, fit direct forecasters, forecast from the last observation, and compare two forecasting models with a cross-validated loss and the Diebold–Mariano test.

Every program is complete and its output is the output of that program.

## Splitting a series

`TsPartition` builds consecutive training and test blocks: the model is always tested on observations that come after its training data. The three constructors are `expanding_window(n, k)` (the training set starts at observation 0 and grows), `sliding_window(n, k)` (the training set has fixed length and moves) and `holdout(n, fraction)` (one split, fraction or count of test observations). `k` is the number of test sets. `PartitionOptions`:

| Option | Meaning | Default |
|---|---|---|
| `test_size` | observations in each test set | see below |
| `step_size` | distance between the starts of consecutive test sets | see below |
| `train_size` / `min_train_size` | training length (sliding) / smallest training length (expanding) | see below |
| `max_train_size` | longest training set of an expanding window | `n − 1` |
| `gap_size` | observations left out between a training set and its test set | 0 |
| `max_lag` | length of the *test buffer*: the observations just before a test set, which hold the lags its first rows need | 0 |
| `direction` | `Reverse` ends the last test set at the end of the series; `Forward` starts the first training set at the beginning | `Reverse` |

The three sizes are linked by `n − train − test − gap = step × (k − 1)`. Whichever are not given are derived from this: with none given all three equal `⌊(n − gap)/(k + 1)⌋`; with only `test_size` or only `step_size` the other equals it and the training size takes what is left; with `train_size` alone the test and step sizes are `⌊(n − gap − train)/k⌋`; with two of the three the third follows from the relation. A combination that does not fit in `n` observations throws.

A window has `train_start`, `train_end`, `test_start`, `test_end`, `test_buffer_start`, `test_buffer_end` (0-based, inclusive), and `partition.training(i)`, `test(i)` and `test_buffer(i)` return the index vectors, `training_mask(i)` and the like Boolean masks.

## The direct forecaster

`DirectForecaster::fit(x, y, options)` takes the predictors `x` (`n × p`; pass an undefined `Series` if there are none) and the response `y` (`n`). `DirectForecasterOptions`:

| Option | Meaning | Default |
|---|---|---|
| `lag.horizon` | the horizons `h` to forecast, one model each | {1} |
| `lag.response_lags` | lags of `y` used as features | {1} |
| `lag.predictor_lags` | for each non-leading predictor, its lags (a list of lists; a single list is applied to all) | lag 1 of every non-leading predictor |
| `lag.leading_predictors` | 0-based indices of predictors whose future values are known | {} |
| `lag.leading_predictor_lags` | their lags; lag 0 is the value at the forecast time `t + h`, lag `ℓ` the value at `t + h − ℓ` | lag 0 |
| `learner` | `OLS` or `Ridge` | OLS |
| `ridge_lambda` | penalty of the ridge learner (the intercept is not penalised) | 1e-6 |
| `fit_intercept` | add a constant | true |

For horizon `h`, row `t` of the training matrix pairs `y_t` with features computed at the origin `t − h`. Only the rows in which every feature exists (after the first few rows, which lack some lag) and the response is observed are used to fit; the first rows of `predict` and `prepared_predictors` are NaN, never zero.

The fitted object offers:

| Method | Result |
|---|---|
| `num_steps()`, `step(i)` | the models; `step(i).horizon`, `step(i).beta` (intercept first, then the features in the order: non-leading predictor lags, response lags, leading predictor lags) |
| `forecast(x, y)` | one forecast per horizon, made from the last observation: a vector of length `num_steps()` |
| `forecast(x, y, x_future)` | the same, when leading predictors reach into the future; `x_future` holds their values after the last row, at least as many rows as the largest horizon |
| `predict(x, y)` | the `n × num_steps()` matrix of fitted values (NaN where lags are missing) |
| `prepared_predictors(x, y, horizon)` | the feature matrix of that horizon |
| `loss(x, y, kind)` | mean squared (`RegressionLoss::MSE`) or absolute (`MAE`) error per horizon over the rows with a prediction |

## Cross-validation and comparison

`DirectCVForecaster::fit(x, y, partition, options)` fits one direct forecaster per training window. `cvpredict(x, y)` returns for each row of a test set the prediction of the model that did not train on it (NaN for rows in no test set); `cvloss(x, y, mode, kind)` returns the loss, averaged over all test rows (`CVLossMode::Average`, one value per horizon) or per fold (`Individual`, folds × horizons).

The Diebold–Mariano test compares two sets of forecast errors `e_A`, `e_B` by the loss difference `d_t = L(e_A,t) − L(e_B,t)`: under the null of equal expected loss its mean is zero. The statistic is the mean divided by its long-run standard error, which includes the autocorrelation of `d_t` up to lag `h − 1` for `h`-step forecasts; with `hln_adjust` (the default) it carries the Harvey–Leybourne–Newbold correction and is referred to a Student t distribution with `n − 1` degrees of freedom, otherwise to a normal. `DmOptions`: `horizon`, `max_lag` (−1 for `horizon − 1`), `loss` (`Squared`, `Absolute`, `Power` with `loss_power`), `hln_adjust`, `tail` (`Both`, `Left`, `Right`), `alpha`. The result `DmResult` has `stat`, `pvalue`, `critical`, `reject`, `mean_diff`, `lrv`, `lags`, `effective_n`. A negative statistic favours the first model.

## Programs

- [Partitions](ts_06_direct_forecasting/partitions.md)
- [Direct forecasts](ts_06_direct_forecasting/direct_forecasts.md)
- [Comparing models out of sample](ts_06_direct_forecasting/comparing_models_out_of_sample.md)

## Checklist

1. Evaluate forecasts on data that follow the training data: use `expanding_window` or `sliding_window`, and a `gap_size` if the target is observed with a delay.
2. Choose `horizon` as the list of steps you need and give each model the lags it can really see at the forecast origin.
3. Declare a predictor *leading* only if its future values are really known when you forecast, and pass them in `x_future`.
4. Compare models with the cross-validated loss per fold and the Diebold–Mariano test; set `horizon` in `DmOptions` to the forecast horizon so the autocorrelation of the errors is accounted for.
5. Remember that NaN marks rows that cannot be predicted; the loss functions ignore them.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

ts_06_direct_forecasting/partitions
ts_06_direct_forecasting/direct_forecasts
ts_06_direct_forecasting/comparing_models_out_of_sample
```
<!-- apiref:toctree:end -->
