# `make_workflow`

## API Signature

- `make_workflow(spec: const ModelSpec&) -> StatusOr<Workflow>`

## Notes

Constructs a high-level workflow object with model and transform stages.

## Inputs

- `spec` (`const ModelSpec&`)
  Method argument; see signature type for accepted form.

## Output

- Returns `StatusOr<Workflow>`. Check `.ok()` first; on success read `.value()` for the computed result.

## Options

Option semantics used by this method:

- `ModelSpec` (`struct`):
  - `TaskKind task = TaskKind::Regression`
    Selects task type (for example classification vs regression).
  - `ModelFamily family = ModelFamily::Linear`
    Selects the model family implementation.
  - `FamilyOptions options = LinearFamilyOptions{}`
    Family-specific options payload used by the selected model/transform family.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto res = make_workflow(/* spec */);
if (!res.ok()) {
  out.error("make_workflow failed", res.status().message);
} else {
  out.info("make_workflow ok");
}
```

**Expected output**

```text
make_workflow: ok
status.ok() == true
result.value() is available
```
