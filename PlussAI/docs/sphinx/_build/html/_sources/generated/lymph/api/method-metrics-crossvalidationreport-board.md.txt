# `metrics::CrossValidationReport::board`

## API Signature

- `metrics::CrossValidationReport::board() -> pluss::table::DataFrame`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `pluss::table::DataFrame`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = metrics::CrossValidationReport::board();
out.info("board", value);
```

**Expected output**

```text
board returned pluss::table::DataFrame
```
