# `Flow::save_blueprint`

## API Signature

- `Flow::save_blueprint() -> StatusOr<std::string>`

## Notes

Persists the current fitted artifact so it can be loaded for inference later.

## Inputs

None.

## Output

- Returns `StatusOr<std::string>`. Check `.ok()` first; on success read `.value()` for the computed result.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto res = Flow::save_blueprint();
if (!res.ok()) {
  out.error("save_blueprint failed", res.status().message);
} else {
  out.info("save_blueprint ok");
}
```

**Expected output**

```text
save_blueprint: ok
status.ok() == true
result.value() is available
```
