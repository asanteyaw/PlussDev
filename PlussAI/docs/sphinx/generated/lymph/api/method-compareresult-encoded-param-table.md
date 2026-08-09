# `CompareResult::encoded_param_table`

## API Signature

- `CompareResult::encoded_param_table(result: const SpecTuneResult&) -> Series`

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
  - `std::vector<Model> fitted{}`
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
auto value = CompareResult::encoded_param_table(/* result */);
out.info("encoded_param_table", value);
```

**Expected output**

```text
encoded_param_table returned Series
```
