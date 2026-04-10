# `OutlierIsolator::max_features_used`

## API Signature

- `OutlierIsolator::max_features_used() -> std::int64_t`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `std::int64_t`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = OutlierIsolator::max_features_used();
out.info("max_features_used", value);
```

**Expected output**

```text
max_features_used: <numeric_result>
```
