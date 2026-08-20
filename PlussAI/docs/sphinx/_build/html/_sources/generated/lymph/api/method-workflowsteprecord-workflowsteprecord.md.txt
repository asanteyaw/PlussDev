# `WorkflowStepRecord::WorkflowStepRecord`

## API Signature

- `branch_index(arg1: in_branch_index), arg2: stage(std::move(in_stage)), arg3: router(std::move(in_router)) -> WorkflowStepRecord(std::int64_t in_branch_index, Stage in_stage, ColumnRouter in_router) :`

## Notes

Instance method on `self`.

## Inputs

- `arg1` (`in_branch_index)`)
  Method argument; see signature type for accepted form.
- `arg2` (`stage(std::move(in_stage))`)
  Method argument; see signature type for accepted form.
- `arg3` (`router(std::move(in_router)`)
  Method argument; see signature type for accepted form.

## Output

- Returns `WorkflowStepRecord(std::int64_t in_branch_index, Stage in_stage, ColumnRouter in_router) :`.

## Options

Option semantics used by this method:

- `WorkflowStepRecord` (`struct`):
  - `std::int64_t branch_index`
    Option field used by this method.
  - `Stage stage`
    Option field used by this method.
  - `ColumnRouter router`
    Option field used by this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = branch_index(/* arg1 */, /* arg2 */, /* arg3 */);
out.info("WorkflowStepRecord", value);
```

**Expected output**

```text
WorkflowStepRecord returned WorkflowStepRecord(std::int64_t in_branch_index, Stage in_stage, ColumnRouter in_router) :
```
