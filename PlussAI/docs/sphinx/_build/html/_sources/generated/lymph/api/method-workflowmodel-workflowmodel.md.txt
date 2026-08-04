# `WorkflowModel::WorkflowModel`

## API Signature

- `WorkflowModel(arg1: const ModelSpec& spec) : workflow_(resolve(make_flow(spec), "make_flow")) -> auto`
- `WorkflowModel(arg1: Workflow workflow) : workflow_(std::move(workflow)) -> auto`

## Notes

Instance method on `self`.

## Inputs

- `arg1` (`Workflow workflow) : workflow_(std::move(workflow) | const ModelSpec& spec) : workflow_(resolve(make_flow(spec), "make_flow")`)
  Method argument; see signature type for accepted form.

## Output

- Returns method result for the selected overload.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = WorkflowModel(/* arg1 */);
out.info("WorkflowModel", value);
```

**Expected output**

```text
WorkflowModel returned result
```
