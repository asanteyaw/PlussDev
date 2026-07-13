# `make_pipeline`

## API Signature

- `make_pipeline(spec: const ModelSpec&, stages: std::initializer_list<TransformSpec>) -> Workflow`
- `make_pipeline(model: WorkflowModel, stages: std::initializer_list<TransformSpec>) -> Workflow`

## Notes

Namespace-level API function.

## Inputs

- `model` (`WorkflowModel`)
  Method argument; see signature type for accepted form.
- `spec` (`const ModelSpec&`)
  Method argument; see signature type for accepted form.
- `stages` (`std::initializer_list<TransformSpec>`)
  List-like argument; pass values in the order expected by the signature.

## Output

- Returns `Workflow`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = make_pipeline(/* spec */, /* stages */);
out.info("make_pipeline", value);
```

**Expected output**

```text
make_pipeline returned Workflow
```
