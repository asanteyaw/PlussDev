# `make_cross_validation_split_options`

## API Signature

- `make_cross_validation_split_options(options: const CrossValidationOptions&) -> SplitOptions`

## Notes

Builds train/test index partitions according to split-plan options.

## Inputs

- `options` (`const CrossValidationOptions&`)
  Configuration object controlling algorithm behavior and safety defaults.

## Output

- Returns `SplitOptions`.

## Options

Option semantics used by this method:

- `CrossValidationOptions` (`struct`):
  - `CVPlanKind plan = CVPlanKind::KFold`
    Controls fold/trial/count sizing.
  - `double holdout_fraction = 0.2`
    Option field used by this method.
  - `std::int64_t folds = 5`
    Controls fold/trial/count sizing.
  - `bool stratified = false`
    Option field used by this method.
  - `bool reliability_defaults = true`
    Option field used by this method.
  - `bool shuffle = true`
    Option field used by this method.
  - `std::uint64_t random_state = 0`
    Random seed for reproducible behavior.
  - `bool deterministic = true`
    Option field used by this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = make_cross_validation_split_options(/* options */);
out.info("make_cross_validation_split_options", value);
```

**Expected output**

```text
split completed
train/test partitions ready
```
