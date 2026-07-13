# `online_factory::objective_series`

## API Signature

- `online_factory::objective_series(result: const SpecTuneResult&) -> Series`

## Notes

Factory helper that constructs a configured model/workflow object.

## Inputs

- `result` (`const SpecTuneResult&`)
  Method argument; see signature type for accepted form.

## Output

- Returns `Series`.

## Options

Option semantics used by this method:

- `SpecTuneResult` (`struct`):
  - `::pluss::ai::nodej::TuneReport report{}`
    Option field used by this method.
  - `ModelSpec best_spec{}`
    Option field used by this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = online_factory::objective_series(/* result */);
out.info("objective_series", value);
```

**Expected output**

```text
objective_series returned Series
```
