# `days`

## API Signature

- `days(value: std::int64_t) -> TimeOffset`

## Notes

Namespace-level API function.

## Inputs

- `value` (`std::int64_t`)
  Method argument; see signature type for accepted form.

## Output

- Returns `TimeOffset`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = days(/* value */);
out.info("days", value);
```

**Expected output**

```text
days returned TimeOffset
```
