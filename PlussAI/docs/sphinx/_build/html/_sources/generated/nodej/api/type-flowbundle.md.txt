# `FlowBundle`

## API Signature

- `FlowBundle`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `FlowBundle` object used by NodeJ/Lymph workflows.

## Notes

Public user-facing type used directly in NodeJ/Lymph workflows.

## Fields

Public fields on this type:

- `FlowBundleMetadata metadata{}`
- `ModelSpec model{}`
- `std::vector<TransformSpec> transforms{}`
- `bool trained = false`
- `bool has_snapshot = false`
- `std::vector<std::vector<Series>> transform_states{}`
- `std::vector<Series> model_state{}`
- `TrainOptions train_options{}`
- `LabeledBatch train_batch{}`
- `CategoryMaps category_maps{}`

## Examples

```cpp
FlowBundle cfg{};
```
