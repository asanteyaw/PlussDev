# `Flow::dump_transform_states`

## API Signature

- `Flow::dump_transform_states() -> StatusOr<std::vector<std::vector<Series>>>`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `StatusOr<std::vector<std::vector<Series>>>`. Check `.ok()` first; on success read `.value()` for the computed result.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto res = Flow::dump_transform_states();
if (!res.ok()) {
  out.error("dump_transform_states failed", res.status().message);
} else {
  out.info("dump_transform_states ok");
}
```

**Expected output**

```text
dump_transform_states: ok
status.ok() == true
result.value() is available
```
