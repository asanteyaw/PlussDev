# `WorkflowModel::graph_info`

## API Signature

- `WorkflowModel::graph_info() -> FlowGraphInfo`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `FlowGraphInfo`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = WorkflowModel::graph_info();
out.info("graph_info", value);
```

**Expected output**

```text
graph_info returned FlowGraphInfo
```
