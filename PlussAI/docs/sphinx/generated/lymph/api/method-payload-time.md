# `Payload::time`

## API Signature

- `Payload::time() -> const Series&`
- `Payload::time(values: Series) -> Payload&`
- `Payload::time(feature_name: std::string) -> Payload&`
- `Payload::time(options: TimeOptions) -> Payload&`

## Notes

Instance method on `self`.

## Inputs

- `feature_name` (`std::string`)
  Input feature data used by this call.
- `options` (`TimeOptions`)
  Configuration object controlling algorithm behavior and safety defaults.
- `values` (`Series`)
  Pluss numeric vector/tensor input.

## Output

- Returns `Payload&`.
- Returns `const Series&`.

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
auto value = Payload::time();
out.info("time", value);
```

**Expected output**

```text
time returned const Series&
```
