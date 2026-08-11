# `Workflow::branches`

## API Signature

- `Workflow::branches() -> const std::vector<WorkflowBranchInfo>&`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `const std::vector<WorkflowBranchInfo>&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Workflow::branches();
out.info("branches", value);
```

**Expected output**

```text
branches returned const std::vector<WorkflowBranchInfo>&
```
