# `search::run`

## API Signature

- `search::run(arg2: const std::function<Model(const GridPoint&) -> Result`
- `search::run(arg2: const std::function<Model(const GridPoint&) -> Result`
- `search::run(arg2: const std::function<Model(const GridPoint&) -> Result`
- `search::run(arg2: const std::function<Model(const GridPoint&) -> Result`
- `search::run(arg2: const std::function<Model(const GridPoint&) -> Result`
- `search::run(arg2: const std::function<Model(const GridPoint&) -> Result`
- `search::run(arg2: const std::function<Model(const GridPoint&) -> Result`
- `search::run(arg2: const std::function<Model(const GridPoint&) -> Result`
- `search::run(arg2: const std::function<Model(const GridPoint&) -> Result`
- `search::run(arg2: const std::function<Model(const GridPoint&) -> Result`

## Notes

Instance method on `self`.

## Inputs

- `arg2` (`const std::function<Model(const GridPoint&`)
  Method argument; see signature type for accepted form.

## Output

- Returns `Result`.

## Options

Option semantics used by this method:

- `search` (`struct`):
  - `selection::CompareReport report{}`
    Option field used by this method.
  - `std::optional<Model> best_model{}`
    Option field used by this method.
  - `std::vector<Model> fitted{}`
    Option field used by this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = search::run(/* arg2 */);
out.info("run", value);
```

**Expected output**

```text
run returned Result
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
run returned Result
```
