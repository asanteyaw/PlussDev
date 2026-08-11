# `Model::reports`

## API Signature

- `Model::reports() -> ::pluss::ai::lymph::reports::ModelReports`
- `Model::reports() -> reports::ModelReports`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `::pluss::ai::lymph::reports::ModelReports`.
- Returns `reports::ModelReports`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Model::reports();
out.info("reports", value);
```

**Expected output**

```text
metrics: <metric_1> <metric_2> ...
evaluation completed
```
