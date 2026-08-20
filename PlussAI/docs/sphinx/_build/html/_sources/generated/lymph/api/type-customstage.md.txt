# `CustomStage`

## API Signature

- `CustomStage`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `CustomStage` object used by NodeJ/Lymph workflows.

## Notes

Public user-facing type used directly in NodeJ/Lymph workflows.

## Fields

Public fields on this type:

- `std::string name{}`
- `SeriesMapFn map_fn{}`
- `prep::FunctionTransformerOptions options{}`
- `TransformContract contract{}`
- `bool persistable = false`

## Examples

```cpp
CustomStage cfg{};
```
