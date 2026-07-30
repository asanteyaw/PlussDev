# `has_best_trial`

## API Signature

- `has_best_trial(report: const TuneReport&) -> bool`

## Notes

Boolean introspection helper that reports whether a capability/state is available.

## Inputs

- `report` (`const TuneReport&`)
  Method argument; see signature type for accepted form.

## Output

- Returns `bool`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = has_best_trial(/* report */);
out.info("has_best_trial", value);
```

**Expected output**

```text
has_best_trial: true
```
