# `has_ledger`

## API Signature

- `has_ledger(report: const TuneReport&) -> bool`

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
auto value = has_ledger(/* report */);
out.info("has_ledger", value);
```

**Expected output**

```text
has_ledger: true
```
