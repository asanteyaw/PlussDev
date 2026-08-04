# `restore`

## API Signature

- `restore(bundle: const FlowBundle&) -> StatusOr<Flow>`

## Notes

Namespace-level API function.

## Inputs

- `bundle` (`const FlowBundle&`)
  Method argument; see signature type for accepted form.

## Output

- Returns `StatusOr<Flow>`. Check `.ok()` first; on success read `.value()` for the computed result.

## Options

Option semantics used by this method:

- `FlowBundle` (`struct`):
  - `FlowBundleMetadata metadata{}`
    Learning-rate step size.
  - `ModelSpec model{}`
    Option field used by this method.
  - `std::vector<TransformSpec> transforms{}`
    Option field used by this method.
  - `bool trained = false`
    Option field used by this method.
  - `bool has_snapshot = false`
    Option field used by this method.
  - `std::vector<std::vector<Series>> transform_states{}`
    Option field used by this method.
  - `std::vector<Series> model_state{}`
    Option field used by this method.
  - `TrainOptions train_options{}`
    Controls fold/trial/count sizing.
  - `LabeledBatch train_batch{}`
    Controls fold/trial/count sizing.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto res = restore(/* bundle */);
if (!res.ok()) {
  out.error("restore failed", res.status().message);
} else {
  out.info("restore ok");
}
```

**Expected output**

```text
restore: ok
status.ok() == true
result.value() is available
```
