# `TuneResult::has_ledger`

## API Signature

- `TuneResult::has_ledger(result: const SpecTuneResult&) -> bool`

## Notes

Boolean introspection helper that reports whether a capability/state is available.

## Inputs

- `result` (`const SpecTuneResult&`)
  Method argument; see signature type for accepted form.

## Output

- Returns `bool`.

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
auto value = TuneResult::has_ledger(/* result */);
out.info("has_ledger", value);
```

**Expected output**

```text
has_ledger: true
```
