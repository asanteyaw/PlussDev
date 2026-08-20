# `SpatialIndex::dim`

## API Signature

- `SpatialIndex::dim() -> std::int64_t`

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
auto value = SpatialIndex::dim();
out.info("dim", value);
```

**Expected output**

```text
dim: <numeric_result>
```
