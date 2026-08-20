# `workflow::workers`

## API Signature

- `workflow::workers() -> std::int64_t`
- `workflow::workers(value: std::int64_t) -> Workflow&`

## Notes

Instance method on `self`.

## Inputs

- `value` (`std::int64_t`)
  Method argument; see signature type for accepted form.

## Output

- Returns `Workflow&`.
- Returns `std::int64_t`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = workflow::workers();
out.info("workers", value);
```

**Expected output**

```text
workers: <numeric_result>
```
