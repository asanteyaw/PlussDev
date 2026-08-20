# `Result::flagged_rate`

## API Signature

- `Result::flagged_rate() -> double`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `double`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Result::flagged_rate();
out.info("flagged_rate", value);
```

**Expected output**

```text
flagged_rate: <numeric_result>
```
