# `OnlineLinearClassifier::OnlineLinearClassifier`

## API Signature

- `OnlineLinearClassifier(arg1: ::pluss::ai::nodej::online::OnlineLinearClassifier core) : core_(std::move(core)) -> auto`

## Notes

Instance method on `self`.

## Inputs

- `arg1` (`::pluss::ai::nodej::online::OnlineLinearClassifier core) : core_(std::move(core)`)
  Method argument; see signature type for accepted form.

## Output

- Returns method result for the selected overload.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = OnlineLinearClassifier(/* arg1 */);
out.info("OnlineLinearClassifier", value);
```

**Expected output**

```text
OnlineLinearClassifier returned result
```

### Example 2 (Demo Pattern)

```cpp
using pluss::Series;
using pluss::ai::nodej::FeatureBatch;
using pluss::ai::nodej::LabeledBatch;
using pluss::ai::nodej::LinearFamilyOptions;
using pluss::ai::nodej::LinearObjective;
using pluss::ai::nodej::ObservationLayout;
using pluss::ai::nodej::RegularizationKind;
using pluss::ai::nodej::TrainOptions;
using pluss::ai::lymph::OnlineLinearClassifier;
using pluss::ai::lymph::OnlineLinearOptions;

Config f64() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

Config i64() {
  return Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU);
}

LabeledBatch slice_rows(const LabeledBatch& batch, std::int64_t start, std::int64_t count) {
  const Series idx = pluss::series::arange(start, start + count, i64());
```

**Expected output**

```text
lymph online seen_samples=1660
lymph online metrics [accuracy]: 0.9875
```
