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

No explicit option struct/enum parameters were detected for this method.

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
