# `Flow::dump_model_state`

## API Signature

- `Flow::dump_model_state() -> StatusOr<std::vector<Series>>`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `StatusOr<std::vector<Series>>`. Check `.ok()` first; on success read `.value()` for the computed result.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto res = Flow::dump_model_state();
if (!res.ok()) {
  out.error("dump_model_state failed", res.status().message);
} else {
  out.info("dump_model_state ok");
}
```

**Expected output**

```text
dump_model_state: ok
status.ok() == true
result.value() is available
```
