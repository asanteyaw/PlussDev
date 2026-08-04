# `best_trial`

## API Signature

- `best_trial(report: const TuneReport&) -> const TuneTrial&`

## Notes

Namespace-level API function.

## Inputs

- `report` (`const TuneReport&`)
  Method argument; see signature type for accepted form.

## Output

- Returns `const TuneTrial&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = best_trial(/* report */);
out.info("best_trial", value);
```

**Expected output**

```text
best_trial returned const TuneTrial&
```
