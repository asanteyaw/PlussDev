# `Flow::set_classifier`

## API Signature

- `Flow::set_classifier(model: std::unique_ptr<ClassifierBase>) -> Status`

## Notes

Sets or updates internal configuration/state used by later train/infer/evaluate calls.

## Inputs

- `model` (`std::unique_ptr<ClassifierBase>`)
  Method argument; see signature type for accepted form.

## Output

- Returns a status object indicating success/failure and an error message on failure.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto st = Flow::set_classifier(/* model */);
if (!st.ok()) {
  out.error("set_classifier failed", st.message);
} else {
  out.info("set_classifier ok");
}
```

**Expected output**

```text
set_classifier: ok
status.ok() == true
```
