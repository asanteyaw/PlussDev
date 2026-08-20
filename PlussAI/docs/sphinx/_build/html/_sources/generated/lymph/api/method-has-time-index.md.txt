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

No explicit option struct/enum parameters were detected for this method.

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
