# `Payload::frame`

## API Signature

- `Payload::frame() -> const pluss::table::DataFrame&`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `const pluss::table::DataFrame&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Payload::frame();
out.info("frame", value);
```

**Expected output**

```text
frame returned const pluss::table::DataFrame&
```

### Example 2 (Demo Pattern)

```cpp
  });
}

} // namespace

int main() {
  std::cout << "NodeJ DataFrame bridge demo\n";

  const DataFrame frame = make_frame();

  DataFrameBatchOptions options{};
  options.feature_names = {"x0", "x1", "category", "ts", "target", "weight"};
  options.label_feature = "target";
  options.sample_weight_feature = "weight";
  options.note_policy = DataNotePolicy::Hash;
  options.thread_policy = DataThreadPolicy::CastTemporalToSeries;

  auto batch_or = labeled_batch_from_dataframe(frame, options);
  if (!require_ok(batch_or, "labeled_batch_from_dataframe")) return 1;

  const LabeledBatch& batch = batch_or.value();
```

**Expected output**

```text
NodeJ DataFrame bridge demo
adapted batch: samples=6 features=4 weighted=yes
linear metrics [mse, r2]: 1.51681e-17 1
NodeJ DataFrame bridge demo completed.
```

### Example 3 (Demo Pattern)

```cpp
    std::int64_t ncases = 6000;
    if (argc > 1 && argv[1] != nullptr && std::string(argv[1]).size() > 0) data_path = fs::path(argv[1]);
    if (argc > 2 && argv[2] != nullptr && std::string(argv[2]).size() > 0) out_dir = fs::path(argv[2]);
    if (argc > 3 && argv[3] != nullptr && std::string(argv[3]).size() > 0) ncases = std::stoll(argv[3]);
    fs::create_directories(out_dir);
    fs::create_directories(model_dir);

    const DataFrame raw = DataFrame::load_csv(data_path.string(), {{"ncases", ncases}});
    auto frame = raw.rename({
        {"dteday", "date"},
        {"holiday", "is_holiday"},
        {"workingday", "is_workingday"},
        {"weathersit", "weather_condition"},
        {"hum", "humidity"},
        {"mnth", "month"},
        {"cnt", "total_count"},
        {"hr", "hour"},
        {"yr", "year"},
    });
```

**Expected output**

```text
CV objective metric: rmse
candidate=linear_ridge objective=102.872 cv_rmse=pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[102.87158] cv_r2=pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[0.41650024] cv_mae=pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[77.611594]
candidate=decision_tree objective=44.9727 cv_rmse=pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[44.972693] cv_r2=pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[0.88843998] cv_mae=pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[27.61723]
candidate=random_forest objective=81.1235 cv_rmse=pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[81.123538] cv_r2=pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[0.63710967] cv_mae=pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
...
```
