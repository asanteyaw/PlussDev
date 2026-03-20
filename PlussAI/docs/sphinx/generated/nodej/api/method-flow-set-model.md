# `Flow::set_model`

## API Signature

- `Flow::set_model(spec: const ModelSpec&) -> Status`

## Notes

Sets or updates internal configuration/state used by later train/infer/evaluate calls.

## Inputs

- `spec` (`const ModelSpec&`)
  Method argument; see signature type for accepted form.

## Output

- Returns a status object indicating success/failure and an error message on failure.

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
auto st = Flow::set_model(/* spec */);
if (!st.ok()) {
  out.error("set_model failed", st.message);
} else {
  out.info("set_model ok");
}
```

**Expected output**

```text
set_model: ok
status.ok() == true
```
