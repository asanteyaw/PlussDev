# `Flow::graph_info`

## API Signature

- `Flow::graph_info() -> StatusOr<FlowGraphInfo>`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `StatusOr<FlowGraphInfo>`. Check `.ok()` first; on success read `.value()` for the computed result.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto res = Flow::graph_info();
if (!res.ok()) {
  out.error("graph_info failed", res.status().message);
} else {
  out.info("graph_info ok");
}
```

**Expected output**

```text
graph_info: ok
status.ok() == true
result.value() is available
```
