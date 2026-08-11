# `Workflow::time`

## API Signature

- `Workflow::time(options: TimeOptions) -> Workflow&`

## Notes

Instance method on `self`.

## Inputs

- `options` (`TimeOptions`)
  Configuration object controlling algorithm behavior and safety defaults.

## Output

- Returns `Workflow&`.

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
auto value = Workflow::time(/* options */);
out.info("time", value);
```

**Expected output**

```text
time returned Workflow&
```
