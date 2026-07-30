# `tune_ledger`

## API Signature

- `tune_ledger(report: const TuneReport&) -> const TuneLedger&`

## Notes

Runs parameter search and returns per-trial metrics plus best-trial selection.

## Inputs

- `report` (`const TuneReport&`)
  Method argument; see signature type for accepted form.

## Output

- Returns `const TuneLedger&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = tune_ledger(/* report */);
out.info("tune_ledger", value);
```

**Expected output**

```text
tune_ledger returned const TuneLedger&
```
