# `validate_time_options`

## API Signature

- `validate_time_options(options: const TimeOptions&, where: const char*) -> void`

## Notes

Validates shape/layout/typing invariants for this data container before downstream use.

## Inputs

- `options` (`const TimeOptions&`)
  Configuration object controlling algorithm behavior and safety defaults.
- `where` (`const char*`)
  Method argument; see signature type for accepted form.

## Output

- No direct return value; operation applies side effects to object state.

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
validate_time_options(/* options */, /* where */);
out.info("validate_time_options completed");
```

**Expected output**

```text
validate_time_options completed
```
