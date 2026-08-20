# `WorkflowBranchSpec`

## API Signature

- `WorkflowBranchSpec`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `WorkflowBranchSpec` object used by NodeJ/Lymph workflows.

## Notes

Public user-facing type used directly in NodeJ/Lymph workflows.

## Fields

Public fields on this type:

- `std::string name{}`
- `std::vector<std::string> columns{}`
- `std::vector<WorkflowStepSpec> steps{}`

## Examples

```cpp
WorkflowBranchSpec cfg{};
```
