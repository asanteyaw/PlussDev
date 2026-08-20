# `CompareResult::tune_ledger`

## API Signature

- `CompareResult::tune_ledger(result: const SpecTuneResult&) -> const ::pluss::ai::nodej::TuneLedger&`

## Notes

Runs parameter search and returns per-trial metrics plus best-trial selection.

## Inputs

- `result` (`const SpecTuneResult&`)
  Method argument; see signature type for accepted form.

## Output

- Returns `const ::pluss::ai::nodej::TuneLedger&`.

## Options

Option semantics used by this method:

- `CompareResult` (`struct`):
  - `CompareReport report{}`
    Option field used by this method.
  - `std::optional<Model> best_model{}`
    Option field used by this method.
  - `std::optional<Model> worst_model{}`
    Option field used by this method.
  - `std::vector<Model> fitted{}`
    Option field used by this method.
  - `std::vector<Model> bottom_fitted{}`
    Option field used by this method.

- `SpecTuneResult` (`struct`):
  - `::pluss::ai::nodej::TuneReport report{}`
    Option field used by this method.
  - `ModelSpec best_spec{}`
    Option field used by this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = CompareResult::tune_ledger(/* result */);
out.info("tune_ledger", value);
```

**Expected output**

```text
tune_ledger returned const ::pluss::ai::nodej::TuneLedger&
```
