# `TimeSeriesSplit::output_render`

## API Signature

- `TimeSeriesSplit::output_render(os: std::ostream&, summary: const CategoryIndexerSummary&) -> void`
- `TimeSeriesSplit::output_render(os: std::ostream&, summary: const OutlierIsolatorSummary&) -> void`
- `TimeSeriesSplit::output_render(os: std::ostream&, summary: const SimpleImputerSummary&) -> void`
- `TimeSeriesSplit::output_render(os: std::ostream&, summary: const StandardScalerSummary&) -> void`
- `TimeSeriesSplit::output_render(os: std::ostream&, summary: const TransformerSummary&) -> void`

## Notes

Instance method on `self`.

## Inputs

- `os` (`std::ostream&`)
  Method argument; see signature type for accepted form.
- `summary` (`const CategoryIndexerSummary& | const OutlierIsolatorSummary& | const SimpleImputerSummary& | const StandardScalerSummary& | const TransformerSummary&`)
  Target values aligned to input observations.

## Output

- No direct return value; operation applies side effects to object state.

## Options

Option semantics used by this method:

- `CategoryIndexerSummary` (`struct`):
  - `bool learned = false`
    Option field used by this method.
  - `std::int64_t feature_count = 0`
    Option field used by this method.
  - `bool include_unknown_bucket = true`
    Controls fold/trial/count sizing.
  - `bool one_based = true`
    Option field used by this method.
  - `std::vector<std::string> feature_names{}`
    Explicit list of feature columns to read from the input frame.
  - `std::vector<std::string> mapped_names{}`
    Option field used by this method.
  - `std::vector<std::vector<std::string>> categories{}`
    Option field used by this method.

- `OutlierIsolatorSummary` (`struct`):
  - `bool learned = false`
    Option field used by this method.
  - `std::int64_t feature_count = 0`
    Option field used by this method.
  - `std::int64_t estimators = 0`
    Option field used by this method.
  - `std::int64_t max_samples = 0`
    Option field used by this method.
  - `std::int64_t max_features = 0`
    Option field used by this method.
  - `bool bootstrap = false`
    Option field used by this method.
  - `bool contamination_auto = true`
    Controls fold/trial/count sizing.
  - `double contamination = 0.1`
    Option field used by this method.
  - `double offset = -0.5`
    Option field used by this method.
  - `std::vector<std::string> feature_names{}`
    Explicit list of feature columns to read from the input frame.

- `SimpleImputerSummary` (`struct`):
  - `SimpleImputerStrategy strategy = SimpleImputerStrategy::Mean`
    Option field used by this method.
  - `bool learned = false`
    Option field used by this method.
  - `std::int64_t feature_count = 0`
    Option field used by this method.
  - `double constant_value = 0.0`
    Option field used by this method.
  - `double empty_feature_fallback = 0.0`
    Option field used by this method.
  - `std::vector<std::string> feature_names{}`
    Explicit list of feature columns to read from the input frame.
  - `Series statistics = Series::undefined()`
    Option field used by this method.

- `StandardScalerSummary` (`struct`):
  - `std::vector<std::string> feature_names{}`
    Explicit list of feature columns to read from the input frame.
  - `Series mean = Series::undefined()`
    Option field used by this method.
  - `Series scale = Series::undefined()`
    Option field used by this method.

- `TimeSeriesSplit` (`struct`):
  - `pluss::table::DataFrame train_set{}`
    Controls fold/trial/count sizing.
  - `pluss::table::DataFrame test_set{}`
    Option field used by this method.
  - `LabeledBatch train_batch{}`
    Controls fold/trial/count sizing.
  - `LabeledBatch test_batch{}`
    Option field used by this method.
  - `IndexSplit indices{}`
    Option field used by this method.

- `TransformerSummary` (`struct`):
  - `std::vector<std::string> feature_names{}`
    Explicit list of feature columns to read from the input frame.
  - `std::vector<Series> state{}`
    Option field used by this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
TimeSeriesSplit::output_render(/* os */, /* summary */);
out.info("output_render completed");
```

**Expected output**

```text
output_render completed
```
