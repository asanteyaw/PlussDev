# `Flow::set_regressor`

## API Signature

- `Flow::set_regressor(model: std::unique_ptr<RegressorBase>) -> Status`

## Notes

Sets or updates internal configuration/state used by later train/infer/evaluate calls.

## Inputs

- `model` (`std::unique_ptr<RegressorBase>`)
  Method argument; see signature type for accepted form.

## Output

- Returns a status object indicating success/failure and an error message on failure.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto st = Flow::set_regressor(/* model */);
if (!st.ok()) {
  out.error("set_regressor failed", st.message);
} else {
  out.info("set_regressor ok");
}
```

**Expected output**

```text
set_regressor: ok
status.ok() == true
```
