# `check_ready_for_infer`

## API Signature

- `check_ready_for_infer(where: const char*) -> Status`

## Notes

Computes predictions from prepared inputs using the currently learned state.

## Inputs

- `where` (`const char*`)
  Method argument; see signature type for accepted form.

## Output

- Returns a status object indicating success/failure and an error message on failure.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto st = check_ready_for_infer(/* where */);
if (!st.ok()) {
  out.error("check_ready_for_infer failed", st.message);
} else {
  out.info("check_ready_for_infer ok");
}
```

**Expected output**

```text
check_ready_for_infer: ok
status.ok() == true
```
