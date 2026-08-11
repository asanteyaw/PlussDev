# `Result::cluster_count`

## API Signature

- `Result::cluster_count() -> std::int64_t`

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
auto value = Result::cluster_count();
out.info("cluster_count", value);
```

**Expected output**

```text
cluster_count: <numeric_result>
```
