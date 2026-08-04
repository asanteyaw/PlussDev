# `Workflow::workflow_model`

## API Signature

- `Workflow::workflow_model() -> const WorkflowModel&`
- `Workflow::workflow_model() -> WorkflowModel&`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `WorkflowModel&`.
- Returns `const WorkflowModel&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Workflow::workflow_model();
out.info("workflow_model", value);
```

**Expected output**

```text
workflow_model returned const WorkflowModel&
```
