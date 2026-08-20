# `make_validation_split_options`

## API Signature

- `make_validation_split_options(options: const metrics::ValidationOptions&) -> SplitOptions`

## Notes

Builds train/test index partitions according to split-plan options.

## Inputs

- `options` (`const metrics::ValidationOptions&`)
  Configuration object controlling algorithm behavior and safety defaults.

## Output

- Returns `SplitOptions`.

## Options

Option semantics used by this method:

- `ValidationOptions` (`struct`):
  - `CVPlanKind plan = CVPlanKind::CrossVal`
    Option field used by this method.
  - `double holdout_fraction = 0.2`
    Option field used by this method.
  - `std::int64_t folds = 5`
    Controls fold/trial/count sizing.
  - `std::int64_t repeats = 1`
    Option field used by this method.
  - `std::int64_t time_gap = 0`
    Option field used by this method.
  - `std::int64_t time_test_size = 0`
    Option field used by this method.
  - `std::int64_t time_max_train_size = 0`
    Controls fold/trial/count sizing.
  - `bool stratified = false`
    Option field used by this method.
  - `Series groups = Series::undefined()`
    Option field used by this method.
  - `bool reliability_defaults = true`
    Option field used by this method.
  - `bool shuffle = true`
    Option field used by this method.
  - `std::int64_t max_parallel = 0`
    Option field used by this method.
  - `std::uint64_t random_state = 0`
    Random seed for reproducible behavior.
  - `bool deterministic = true`
    Option field used by this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = make_validation_split_options(/* options */);
out.info("make_validation_split_options", value);
```

**Expected output**

```text
split completed
train/test partitions ready
```
