# `workflow::branch_count`

## API Signature

- `workflow::branch_count() -> std::int64_t`

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
auto value = workflow::branch_count();
out.info("branch_count", value);
```

**Expected output**

```text
branch_count: <numeric_result>
```
