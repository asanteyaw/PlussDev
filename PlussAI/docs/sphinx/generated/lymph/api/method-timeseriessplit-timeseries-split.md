# `TimeSeriesSplit::timeseries_split`

## API Signature

- `TimeSeriesSplit::timeseries_split(frame: const pluss::table::DataFrame&, time_feature: std::string_view, options: TimeSeriesSplitOptions) -> std::vector<TimeSeriesSplit>`

## Notes

Builds train/test index partitions according to split-plan options.

## Inputs

- `frame` (`const pluss::table::DataFrame&`)
  Input feature data used by this call.
- `options` (`TimeSeriesSplitOptions`)
  Configuration object controlling algorithm behavior and safety defaults.
- `time_feature` (`std::string_view`)
  Input feature data used by this call.

## Output

- Returns `std::vector<TimeSeriesSplit>`.

## Options

Option semantics used by this method:

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

- `TimeSeriesSplitOptions` (`struct`):
  - `std::int64_t folds = 5`
    Controls fold/trial/count sizing.
  - `std::int64_t gap = 0`
    Option field used by this method.
  - `std::int64_t test_size = 0`
    Option field used by this method.
  - `std::int64_t max_train_size = 0`
    Controls fold/trial/count sizing.
  - `std::uint64_t random_state = 0`
    Random seed for reproducible behavior.
  - `bool deterministic = true`
    Option field used by this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = TimeSeriesSplit::timeseries_split(/* frame */, /* time_feature */, /* options */);
out.info("timeseries_split", value);
```

**Expected output**

```text
split completed
train/test partitions ready
```
