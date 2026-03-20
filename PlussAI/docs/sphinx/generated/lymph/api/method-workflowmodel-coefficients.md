# `WorkflowModel::coefficients`

## API Signature

- `WorkflowModel::coefficients() -> std::vector<double>`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `std::vector<double>`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = WorkflowModel::coefficients();
out.info("coefficients", value);
```

**Expected output**

```text
coefficients returned std::vector<double>
```
