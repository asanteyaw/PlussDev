# `Flow::set_nested_param`

## API Signature

- `Flow::set_nested_param(path: std::string_view, value: double) -> Status`

## Notes

Sets or updates internal configuration/state used by later train/infer/evaluate calls.

## Inputs

- `path` (`std::string_view`)
  Path to persisted model/data artifact.
- `value` (`double`)
  Method argument; see signature type for accepted form.

## Output

- Returns a status object indicating success/failure and an error message on failure.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto st = Flow::set_nested_param(/* path */, /* value */);
if (!st.ok()) {
  out.error("set_nested_param failed", st.message);
} else {
  out.info("set_nested_param ok");
}
```

**Expected output**

```text
set_nested_param: ok
status.ok() == true
```
