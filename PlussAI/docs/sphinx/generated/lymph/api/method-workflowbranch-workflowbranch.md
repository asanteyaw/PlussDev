# `WorkflowBranch::WorkflowBranch`

## API Signature

- `workflow_(arg1: &workflow), arg2: name_(std::move(name)) -> WorkflowBranch(Workflow& workflow, std::string name) :`

## Notes

Instance method on `self`.

## Inputs

- `arg1` (`&workflow)`)
  Method argument; see signature type for accepted form.
- `arg2` (`name_(std::move(name)`)
  Method argument; see signature type for accepted form.

## Output

- Returns `WorkflowBranch(Workflow& workflow, std::string name) :`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = workflow_(/* arg1 */, /* arg2 */);
out.info("WorkflowBranch", value);
```

**Expected output**

```text
WorkflowBranch returned WorkflowBranch(Workflow& workflow, std::string name) :
```
