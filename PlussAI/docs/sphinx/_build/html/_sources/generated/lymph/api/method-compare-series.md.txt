# `compare_series`

## API Signature

- `compare_series(lhs: const Series&, rhs: const Series&) -> SeriesCompareReport`

## Notes

Namespace-level API function.

## Inputs

- `lhs` (`const Series&`)
  Pluss numeric vector/tensor input.
- `rhs` (`const Series&`)
  Pluss numeric vector/tensor input.

## Output

- Returns `SeriesCompareReport`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = compare_series(/* lhs */, /* rhs */);
out.info("compare_series", value);
```

**Expected output**

```text
metrics: <metric_1> <metric_2> ...
evaluation completed
```
