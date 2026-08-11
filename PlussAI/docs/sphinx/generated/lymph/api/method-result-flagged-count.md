# `Result::flagged_count`

## API Signature

- `Result::flagged_count() -> std::int64_t`

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
auto value = Result::flagged_count();
out.info("flagged_count", value);
```

**Expected output**

```text
flagged_count: <numeric_result>
```
