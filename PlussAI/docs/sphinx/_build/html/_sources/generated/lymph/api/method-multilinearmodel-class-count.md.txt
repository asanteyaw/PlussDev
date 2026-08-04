# `MultiLinearModel::class_count`

## API Signature

- `MultiLinearModel::class_count() -> std::int64_t`

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
auto value = MultiLinearModel::class_count();
out.info("class_count", value);
```

**Expected output**

```text
class_count: <numeric_result>
```
