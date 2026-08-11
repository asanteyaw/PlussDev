# `WorkflowRoute::WorkflowRoute`

## API Signature

- `workflow_(arg1: &workflow), arg2: router_(std::move(router)), branch_index: branch_index_() -> WorkflowRoute(Workflow& workflow, ColumnRouter router, std::int64_t branch_index) :`

## Notes

Instance method on `self`.

## Inputs

- `arg1` (`&workflow)`)
  Method argument; see signature type for accepted form.
- `arg2` (`router_(std::move(router))`)
  Method argument; see signature type for accepted form.
- `branch_index` (`branch_index_(`)
  Input feature data used by this call.

## Output

- Returns `WorkflowRoute(Workflow& workflow, ColumnRouter router, std::int64_t branch_index) :`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = workflow_(/* arg1 */, /* arg2 */, /* branch_index */);
out.info("WorkflowRoute", value);
```

**Expected output**

```text
WorkflowRoute returned WorkflowRoute(Workflow& workflow, ColumnRouter router, std::int64_t branch_index) :
```
