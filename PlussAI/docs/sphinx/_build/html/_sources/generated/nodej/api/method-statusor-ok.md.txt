# `StatusOr::ok`

## API Signature

- `StatusOr::ok() -> bool`

## Notes

Boolean state check for object readiness.

## Inputs

None.

## Output

- Returns `bool`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = StatusOr::ok();
out.info("ok", value);
```

**Expected output**

```text
ok: true
```
