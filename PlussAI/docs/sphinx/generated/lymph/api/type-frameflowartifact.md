# `FrameFlowArtifact`

## API Signature

- `FrameFlowArtifact`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `FrameFlowArtifact` object used by NodeJ/Lymph workflows.

## Notes

Public user-facing type used directly in NodeJ/Lymph workflows.

## Fields

Public fields on this type:

- `nodej::Flow flow{}`
- `nodej::Series metrics = nodej::Series::undefined()`
- `nodej::LabeledBatch train_batch{}`

## Examples

```cpp
FrameFlowArtifact cfg{};
```
