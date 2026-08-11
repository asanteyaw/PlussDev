# `WorkflowStepRecord`

## API Signature

- `WorkflowStepRecord`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `WorkflowStepRecord` object used by NodeJ/Lymph workflows.

## Notes

Public user-facing type used directly in NodeJ/Lymph workflows.

## Fields

Public fields on this type:

- `std::int64_t branch_index`
- `Stage stage`
- `ColumnRouter router`

## Examples

```cpp
WorkflowStepRecord cfg{};
// cfg.branch_index = ...;
```
