# `CrossValidationReport::board`

## API Signature

- `CrossValidationReport::board() -> pluss::table::DataFrame`
- `CrossValidationReport::board() -> pluss::table::DataFrame`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `pluss::table::DataFrame`.

## Options

Option semantics used by this method:

- `CrossValidationReport` (`struct`):
  - `CrossAssessReport report{}`
    Option field used by this method.
  - `MetricReport mean{}`
    Selects optimization or evaluation objective.
  - `MetricReport stdev{}`
    Selects optimization or evaluation objective.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = CrossValidationReport::board();
out.info("board", value);
```

**Expected output**

```text
board returned pluss::table::DataFrame
```
