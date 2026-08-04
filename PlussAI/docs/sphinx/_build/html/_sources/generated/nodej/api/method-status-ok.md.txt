# `Status::ok`

## API Signature

- `Status::ok() -> bool`

## Notes

Boolean state check for object readiness.

## Inputs

None.

## Output

- Returns `bool`.

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
auto value = Status::ok();
out.info("ok", value);
```

**Expected output**

```text
ok: true
```
