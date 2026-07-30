# `TuneResult::tune_ledger`

## API Signature

- `TuneResult::tune_ledger(result: const SpecTuneResult&) -> const ::pluss::ai::nodej::TuneLedger&`

## Notes

Runs parameter search and returns per-trial metrics plus best-trial selection.

## Inputs

- `result` (`const SpecTuneResult&`)
  Method argument; see signature type for accepted form.

## Output

- Returns `const ::pluss::ai::nodej::TuneLedger&`.

## Options

Option semantics used by this method:

- `SpecTuneResult` (`struct`):
  - `::pluss::ai::nodej::TuneReport report{}`
    Option field used by this method.
  - `ModelSpec best_spec{}`
    Option field used by this method.

- `TuneResult` (`struct`):
  - `TuneReport report{}`
    Option field used by this method.
  - `Workflow best_workflow{}`
    Option field used by this method.
  - `std::vector<Workflow> fitted{}`
    Option field used by this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = TuneResult::tune_ledger(/* result */);
out.info("tune_ledger", value);
```

**Expected output**

```text
tune_ledger returned const ::pluss::ai::nodej::TuneLedger&
```
