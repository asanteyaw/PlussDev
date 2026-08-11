# `Workflow::branch`

## API Signature

- `Workflow::branch(name: std::string) -> WorkflowBranch`
- `Workflow::branch(name: std::string) -> WorkflowBranch`

## Notes

Instance method on `self`.

## Inputs

- `name` (`std::string`)
  Method argument; see signature type for accepted form.

## Output

- Returns `WorkflowBranch`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Workflow::branch(/* name */);
out.info("branch", value);
```

**Expected output**

```text
branch returned WorkflowBranch
```
