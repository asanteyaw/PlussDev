# `Flow::clone`

## API Signature

- `Flow::clone() -> StatusOr<Flow>`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `StatusOr<Flow>`. Check `.ok()` first; on success read `.value()` for the computed result.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto res = Flow::clone();
if (!res.ok()) {
  out.error("clone failed", res.status().message);
} else {
  out.info("clone ok");
}
```

**Expected output**

```text
clone: ok
status.ok() == true
result.value() is available
```
