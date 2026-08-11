# `has_time_index`

## API Signature

- `has_time_index(options: const TimeOptions&) -> bool`

## Notes

Boolean introspection helper that reports whether a capability/state is available.

## Inputs

- `options` (`const TimeOptions&`)
  Configuration object controlling algorithm behavior and safety defaults.

## Output

- Returns `bool`.

## Options

Option semantics used by this method:

- `TimeOptions` (`struct`):
  - `std::string index{}`
    Option field used by this method.
  - `std::string observed_at{}`
    Option field used by this method.
  - `std::string target_at{}`
    Option field used by this method.
  - `TimeOffset horizon = 0`
    Option field used by this method.
  - `TimeOffset gap = 0`
    Option field used by this method.
  - `TimeOffset lookback = 0`
    Option field used by this method.
  - `bool require_ordered = true`
    Option field used by this method.
  - `bool reject_missing_index = true`
    Option field used by this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = has_time_index(/* options */);
out.info("has_time_index", value);
```

**Expected output**

```text
has_time_index: true
```
