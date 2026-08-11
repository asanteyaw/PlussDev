# `Search::run`

## API Signature

- `Search::run(make_model: const std::function<Model(const GridPoint&)>&, batch: const LabeledBatch&, split_options: const SplitOptions&, train_options: const TrainOptions&, assess_options_in: const AssessOptions&, objective_metric: MetricKind, objective_policy: TuneObjectivePolicy, fit_options: selection::FitOptions) -> SearchResult`
- `Search::run(arg2: const std::function<Model(const GridPoint&) -> SearchResult`
- `Search::run(arg2: const std::function<Model(const GridPoint&) -> SearchResult`
- `Search::run(arg2: const std::function<Model(const GridPoint&) -> SearchResult`
- `Search::run(arg2: const std::function<Model(const GridPoint&) -> SearchResult`
- `Search::run(arg2: const std::function<Model(const GridPoint&) -> SearchResult`
- `Search::run(arg2: const std::function<Model(const GridPoint&) -> SearchResult`
- `Search::run(arg2: const std::function<Model(const GridPoint&) -> SearchResult`
- `Search::run(arg2: const std::function<Model(const GridPoint&) -> SearchResult`

## Notes

Instance method on `self`.

## Inputs

- `arg2` (`const std::function<Model(const GridPoint&`)
  Method argument; see signature type for accepted form.
- `assess_options_in` (`const AssessOptions&`)
  Configuration object controlling algorithm behavior and safety defaults.
- `batch` (`const LabeledBatch&`)
  Input feature data used by this call.
- `fit_options` (`selection::FitOptions`)
  Configuration object controlling algorithm behavior and safety defaults.
- `make_model` (`const std::function<Model(const GridPoint&)>&`)
  Algorithm-size/count argument used to shape fit or split behavior.
- `objective_metric` (`MetricKind`)
  Metric selection or score-control argument for evaluation/tuning.
- `objective_policy` (`TuneObjectivePolicy`)
  Target values aligned to input observations.
- `split_options` (`const SplitOptions&`)
  Configuration object controlling algorithm behavior and safety defaults.
- `train_options` (`const TrainOptions&`)
  Configuration object controlling algorithm behavior and safety defaults.

## Output

- Returns `SearchResult`.

## Options

Option semantics used by this method:

- `FitOptions` (`struct`):
  - `std::int64_t fit_top = 0`
    Option field used by this method.
  - `std::int64_t fit_bottom = 0`
    Option field used by this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Search::run(/* make_model */, /* batch */, /* split_options */, /* train_options */, /* assess_options_in */, /* objective_metric */, /* objective_policy */, /* fit_options */);
out.info("run", value);
```

**Expected output**

```text
run returned SearchResult
```

### Example 2 (Demo Pattern)

```cpp
      .hist("residual", 80)
      .headline("Residual distribution (final model)")
      .save_pdf((out_dir / "final_residual_hist.pdf").string());
  pred_frame.plot()
      .hist("abs_error", 80)
      .headline("Absolute error distribution (final model)")
      .save_pdf((out_dir / "final_abs_error_hist.pdf").string());

  //========== run summary ========
  // This section writes compact run metadata.
  DataFrame run_summary({
      {"n_rows_loaded", Thread(pluss::make_series(std::vector<double>{static_cast<double>(frame.ncases())}, {1}, Config{}.with_dtype(ScalarType::Float64)))} ,
      {"n_features_used", Thread(pluss::make_series(std::vector<double>{static_cast<double>(model_feature_names.size())}, {1}, Config{}.with_dtype(ScalarType::Float64)))} ,
      {"holdout_train_rows", Thread(pluss::make_series(std::vector<double>{static_cast<double>(train_frame_full.ncases())}, {1}, Config{}.with_dtype(ScalarType::Float64)))} ,
      {"holdout_test_rows", Thread(pluss::make_series(std::vector<double>{static_cast<double>(test_frame_full.ncases())}, {1}, Config{}.with_dtype(ScalarType::Float64)))} ,
      {"broad_candidates", Thread(pluss::make_series(std::vector<double>{static_cast<double>(broad_specs.size())}, {1}, Config{}.with_dtype(ScalarType::Float64)))} ,
      {"tuned_candidates", Thread(pluss::make_series(std::vector<double>{static_cast<double>(tuned_specs.size())}, {1}, Config{}.with_dtype(ScalarType::Float64)))} ,
      {"final_rmse", Thread(pluss::make_series(std::vector<double>{final_metrics_vec.size() > 0 ? final_metrics_vec[0] : std::numeric_limits<double>::quiet_NaN()}, {1}, Config{}.with_dtype(ScalarType::Float64)))} ,
      {"final_r2", Thread(pluss::make_series(std::vector<double>{final_metrics_vec.size() > 1 ? final_metrics_vec[1] : std::numeric_limits<double>::quiet_NaN()}, {1}, Config{}.with_dtype(ScalarType::Float64)))} ,
      {"final_mae", Thread(pluss::make_series(std::vector<double>{final_metrics_vec.size() > 2 ? final_metrics_vec[2] : std::numeric_limits<double>::quiet_NaN()}, {1}, Config{}.with_dtype(ScalarType::Float64)))} ,
      {"restored_max_abs_diff", Thread(pluss::make_series(std::vector<double>{restored_max_abs_diff}, {1}, Config{}.with_dtype(ScalarType::Float64)))} ,
```

**Expected output**

```text
run returned SearchResult
```
