# `Status::success`

## API Signature

- `Status::success() -> Status`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns a status object indicating success/failure and an error message on failure.

## Options

Option semantics used by this method:

- `Status` (`struct`):
  - `StatusCode code = StatusCode::Ok`
    Option field used by this method.
  - `std::string message{}`
    Option field used by this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto st = Status::success();
if (!st.ok()) {
  out.error("success failed", st.message);
} else {
  out.info("success ok");
}
```

**Expected output**

```text
success: ok
status.ok() == true
```
