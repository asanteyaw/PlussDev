# `CompareResult::objective_series`

## API Signature

- `CompareResult::objective_series(result: const SpecTuneResult&) -> Series`

## Notes

Instance method on `self`.

## Inputs

- `result` (`const SpecTuneResult&`)
  Method argument; see signature type for accepted form.

## Output

- Returns `Series`.

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
auto value = CompareResult::objective_series(/* result */);
out.info("objective_series", value);
```

**Expected output**

```text
objective_series returned Series
```
