# `HoldoutPlan::stratify`

## API Signature

- `HoldoutPlan::stratify(value: Series) -> HoldoutPlan&`
- `HoldoutPlan::stratify(feature_name: std::string) -> HoldoutPlan&`

## Notes

Instance method on `self`.

## Inputs

- `feature_name` (`std::string`)
  Input feature data used by this call.
- `value` (`Series`)
  Pluss numeric vector/tensor input.

## Output

- Returns `HoldoutPlan&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = HoldoutPlan::stratify(/* value */);
out.info("stratify", value);
```

**Expected output**

```text
stratify returned HoldoutPlan&
```
