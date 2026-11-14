<!-- generated from partitions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Partitions

[Back to Direct Forecasting and Model Comparison](../ts_06_direct_forecasting.md)

The only honest test of a forecasting model is how it forecasts data it has not seen. For a series that means a split in *time order*: the model is trained on an early stretch and tested on a later one. A random split, the default in much of machine learning, is wrong here: it puts observations from the future in the training set and tests on points whose neighbours the model has already seen, which makes autocorrelated series look far more predictable than they are. `TsPartition` builds time-ordered training and test blocks, and everything else in this chapter (the direct forecaster's cross-validation, the model comparison) is built on top of it.

There are three constructors:

- `expanding_window(n, k)`: `k` test sets, each preceded by a training set that starts at the first observation and *grows*. It mimics a forecaster who keeps all the history.
- `sliding_window(n, k)`: `k` test sets, each preceded by a training set of fixed length that *moves*. It mimics a forecaster who uses only the recent past, which suits series whose behaviour changes.
- `holdout(n, fraction)`: one split, with the last `fraction` of the series held out (a count if you pass an integer).

`PartitionOptions` fine-tunes the windows: `test_size`, `step_size` (how far consecutive test sets move), `train_size` or `min_train_size`, `max_train_size`, `gap_size` (observations left out between a training set and its test set), `max_lag` (the length of a *test buffer* that holds the lags a test set's first rows need), and `direction` (`Reverse` ends the last test set at the end of the series, `Forward` starts the first training set at the beginning). The three sizes are linked by `n − train − test − gap = step × (k − 1)`; give the ones you care about and the rest follow. A combination that cannot fit in `n` observations throws.

A window records `train_start`, `train_end`, `test_start`, `test_end` (0-based, inclusive) and the buffer, and `partition.training(i)`, `test(i)` and `test_buffer(i)` return the index vectors (`training_mask(i)` and friends return Boolean masks).

## The three window types

The first program prints the windows of each constructor for 100 observations and three test sets (positions are shown 1-based).

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace ts = pluss::subsp::stats::ts;
using namespace pluss::tils;

int main() {
  OutputEngine out;

  out.writeln("expanding window");
  const auto partition1 = ts::TsPartition::expanding_window(100, 3);
  for (const auto& w : partition1.windows())
    out.writeln("  train", w.train_start + 1, "-", w.train_end + 1, "(", w.train_size(), "obs )  test", w.test_start + 1, "-", w.test_end + 1, "(", w.test_size(), "obs )");

  out.writeln("sliding window");
  const auto partition2 = ts::TsPartition::sliding_window(100, 3);
  for (const auto& w : partition2.windows())
    out.writeln("  train", w.train_start + 1, "-", w.train_end + 1, "(", w.train_size(), "obs )  test", w.test_start + 1, "-", w.test_end + 1, "(", w.test_size(), "obs )");

  out.writeln("holdout, last 20%");
  const auto partition3 = ts::TsPartition::holdout(100, 0.2);
  for (const auto& w : partition3.windows())
    out.writeln("  train", w.train_start + 1, "-", w.train_end + 1, "(", w.train_size(), "obs )  test", w.test_start + 1, "-", w.test_end + 1, "(", w.test_size(), "obs )");
}
```

Output

```text
expanding window
  train 1 - 25 ( 25 obs )  test 26 - 50 ( 25 obs )
  train 1 - 50 ( 50 obs )  test 51 - 75 ( 25 obs )
  train 1 - 75 ( 75 obs )  test 76 - 100 ( 25 obs )
sliding window
  train 1 - 25 ( 25 obs )  test 26 - 50 ( 25 obs )
  train 26 - 50 ( 25 obs )  test 51 - 75 ( 25 obs )
  train 51 - 75 ( 25 obs )  test 76 - 100 ( 25 obs )
holdout, last 20%
  train 1 - 80 ( 80 obs )  test 81 - 100 ( 20 obs )
```

By default the training length, the test length and the step are each a quarter of the series. In the expanding window the training sets are nested (25, 50 and 75 observations); in the sliding window they all have the same length. The last test set ends at the end of the series, so the most recent data are always used for testing. The holdout is a single split of 80 and 20.

## Gaps, sizes and direction

The second program shows how the options change the windows. A **gap** leaves the observations just before each test set unused. It matters when the target is observed with a delay, or when the forecast horizon is long: a model asked to forecast ten steps ahead must not have been trained on observations that overlap its test period. The sizes are recomputed so that the windows still fit. Giving only `test_size` sets the step equal to it and lets the training set take the rest. `Forward` anchors the first training set at the start of the series instead of ending the last test set at the end.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace ts = pluss::subsp::stats::ts;
using namespace pluss::tils;

int main() {
  OutputEngine out;

  ts::PartitionOptions gap;
  gap.gap_size = 2;
  out.writeln("expanding window with a gap of 2");
  const auto partition4 = ts::TsPartition::expanding_window(100, 3, gap);
  for (const auto& w : partition4.windows())
    out.writeln("  train", w.train_start + 1, "-", w.train_end + 1, "  test", w.test_start + 1, "-", w.test_end + 1);

  ts::PartitionOptions test10;
  test10.test_size = 10;
  out.writeln("sliding window, test size 10 (step = 10, the training window takes the rest)");
  const auto partition5 = ts::TsPartition::sliding_window(100, 4, test10);
  for (const auto& w : partition5.windows())
    out.writeln("  train", w.train_start + 1, "-", w.train_end + 1, "  test", w.test_start + 1, "-", w.test_end + 1);

  ts::PartitionOptions forward;
  forward.direction = ts::Direction::Forward;
  forward.min_train_size = 40;
  forward.step_size = 20;
  out.writeln("expanding window, forward from a training set of 40, step 20");
  const auto partition6 = ts::TsPartition::expanding_window(100, 3, forward);
  for (const auto& w : partition6.windows())
    out.writeln("  train", w.train_start + 1, "-", w.train_end + 1, "  test", w.test_start + 1, "-", w.test_end + 1);

  ts::PartitionOptions lag;
  lag.max_lag = 5;
  const auto w = ts::TsPartition::expanding_window(100, 3, lag).window(0);
  out.writeln("with max_lag = 5 the first test set", w.test_start + 1, "-", w.test_end + 1, "gets the buffer", w.test_buffer_start + 1, "-", w.test_buffer_end + 1);
}
```

Output

```text
expanding window with a gap of 2
  train 1 - 26   test 29 - 52
  train 1 - 50   test 53 - 76
  train 1 - 74   test 77 - 100
sliding window, test size 10 (step = 10, the training window takes the rest)
  train 1 - 60   test 61 - 70
  train 11 - 70   test 71 - 80
  train 21 - 80   test 81 - 90
  train 31 - 90   test 91 - 100
expanding window, forward from a training set of 40, step 20
  train 1 - 40   test 41 - 60
  train 1 - 60   test 61 - 80
  train 1 - 80   test 81 - 100
with max_lag = 5 the first test set 26 - 50 gets the buffer 21 - 25
```

With a gap of 2 each training set ends two observations before its test set (training 1–26, test 29–52) and the test sets shrink from 25 to 24 observations so the windows still fit; with `test_size = 10` the step is 10 and the sliding training window is 60 observations. The `Forward` windows start from a training set of 40 and move by the step of 20. The **test buffer** is the least obvious option: a model that uses the last five values as features cannot predict the first five observations of a test set unless those features are available, so `max_lag` reserves them just before the test set. They are inputs only, never scored.

## Building a backtest by hand

A partition is also a plain description of folds, so you can run any model through it. The program below evaluates a simple forecaster, an AR(1) fitted on each training window and used to forecast the whole test window from the end of training, and compares it with a naive forecast (the training mean) on the same folds.

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

int main() {
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  ts::ArimaFit truth;
  truth.ar = {0.8};
  truth.intercept = 2.0 * (1.0 - 0.8);  // reverts to a mean of 2
  const int n = 300;
  const auto y = truth.simulate(n, f64, /*seed=*/14).host<double>();

  const auto partition = ts::TsPartition::expanding_window(n, 5);
  std::cout << std::fixed << std::setprecision(3);
  std::cout << std::setw(6) << "fold" << std::setw(10) << "train" << std::setw(8) << "test" << std::setw(14) << "AR(1) RMSE" << std::setw(14) << "mean RMSE" << "\n";
  for (std::int64_t i = 0; i < partition.num_test_sets(); ++i) {
    const auto& w = partition.window(i);
    const Series train = pluss::make_series(std::vector<double>(y.begin() + w.train_start, y.begin() + w.train_end + 1), f64);
    ts::ArimaOptions o;
    o.ar_order = 1;
    const auto fit = ts::Arima::fit_mle(train, o, f64);
    const auto fc = fit.forecast(w.test_size(), train, f64).host<double>();
    double mean = 0.0;
    for (std::int64_t t = w.train_start; t <= w.train_end; ++t) mean += y[t];
    mean /= static_cast<double>(w.train_size());
    double se_ar = 0.0, se_mean = 0.0;
    for (std::int64_t k = 0; k < w.test_size(); ++k) {
      const double actual = y[w.test_start + k];
      se_ar += (actual - fc[k]) * (actual - fc[k]);
      se_mean += (actual - mean) * (actual - mean);
    }
    std::cout << std::setw(6) << i + 1 << std::setw(10) << w.train_size() << std::setw(8) << w.test_size() << std::setw(14) << std::sqrt(se_ar / w.test_size()) << std::setw(14)
              << std::sqrt(se_mean / w.test_size()) << "\n";
  }
}
```

Output

```text
  fold     train    test    AR(1) RMSE     mean RMSE
     1        50      50         1.372         1.960
     2       100      50         1.558         1.698
     3       150      50         1.335         1.300
     4       200      50         1.918         1.842
     5       250      50         1.738         1.824
```

The AR(1) forecast of a whole 50-observation test window from its first observation decays to the mean within a few steps (the coefficient is 0.8, so the memory is about ten steps), so over most of the window it is no better than the mean itself, and the two errors are close: the AR(1) is better in three folds (1.37 against 1.96 in the first, where the training set is only 50 observations and the mean is poorly estimated) and the mean in two (1.30 against 1.34 in the third). The lesson is the *method*, not the winner: fit on `training(i)`, score on `test(i)`, repeat over folds, and look at the spread across folds as well as the average, because a single split can flatter or punish a model by chance. Here a single fold would have said "the AR(1) is 30% better" or "the mean is better" depending on which one you looked at.

## Decisions and pitfalls

- **Never shuffle.** Training data must precede test data, always; otherwise the test is leaked.
- **Match the split to how the model will be used.** Expanding windows for a forecaster that keeps all history, sliding windows for one that keeps only recent data.
- **Use a gap for long horizons or delayed targets.** A model forecasting `h` steps ahead should not train on the `h − 1` observations before the test set.
- **Reserve the buffer.** If features use lags, set `max_lag` so the first test rows have their inputs.
- **Several folds, not one.** Report the average and the spread; one fold can be lucky.

Where next: [Direct forecasts](direct_forecasts.md) fits the forecasting models that these windows are used to evaluate.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="direct_forecasts.html">
    <span class="label">Next</span>
    <span class="title">Direct forecasts</span>
  </a>
</div>
<!-- apiref:nav:end -->
