# `WorkflowLabeledInput`

## API Signature

- `WorkflowLabeledInput`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `WorkflowLabeledInput` object used by NodeJ/Lymph workflows.

## Notes

Public user-facing type used directly in NodeJ/Lymph workflows.

## Fields

Public fields on this type:

- `std::optional<pluss::table::DataFrame> frame{}`
- `DataFrameBatchOptions data_options{}`
- `std::optional<LabeledBatch> batch{}`

## Examples

```cpp
WorkflowLabeledInput cfg{};
```
