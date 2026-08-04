# `WorkflowModel::intercept_term`

## API Signature

- `WorkflowModel::intercept_term() -> double`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `double`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = WorkflowModel::intercept_term();
out.info("intercept_term", value);
```

**Expected output**

```text
intercept_term: <numeric_result>
```
