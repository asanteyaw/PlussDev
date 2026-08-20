# `TimeSeriesSplit::make_holdout_split_options`

## API Signature

- `TimeSeriesSplit::make_holdout_split_options(options: const HoldoutSplitOptions&) -> SplitOptions`

## Notes

Builds train/test index partitions according to split-plan options.

## Inputs

- `options` (`const HoldoutSplitOptions&`)
  Configuration object controlling algorithm behavior and safety defaults.

## Output

- Returns `SplitOptions`.

## Options

Option semantics used by this method:

- `HoldoutSplitOptions` (`struct`):
  - `double test_size = 0.2`
    Option field used by this method.
  - `Series stratify = Series::undefined()`
    Option field used by this method.
  - `std::string stratify_feature{}`
    Option field used by this method.
  - `std::uint64_t random_state = 0`
    Random seed for reproducible behavior.
  - `bool deterministic = true`
    Option field used by this method.
  - `bool shuffle = true`
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

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = TimeSeriesSplit::make_holdout_split_options(/* options */);
out.info("make_holdout_split_options", value);
```

**Expected output**

```text
split completed
train/test partitions ready
```
