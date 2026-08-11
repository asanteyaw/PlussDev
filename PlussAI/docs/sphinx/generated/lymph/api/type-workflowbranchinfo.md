# `WorkflowBranchInfo`

## API Signature

- `WorkflowBranchInfo`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `WorkflowBranchInfo` object used by NodeJ/Lymph workflows.

## Notes

Public user-facing type used directly in NodeJ/Lymph workflows.

## Fields

Public fields on this type:

- `std::string name{}`
- `bool named = false`
- `std::int64_t stage_count = 0`

## Examples

```cpp
WorkflowBranchInfo cfg{};
```
