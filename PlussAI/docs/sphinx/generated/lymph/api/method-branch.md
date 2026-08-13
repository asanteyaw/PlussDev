# `branch`

## API Signature

- `branch(name: std::string, columns: std::initializer_list<std::string>, steps: std::initializer_list<WorkflowStepSpec>) -> WorkflowBranchSpec`
- `branch(name: std::string, columns: std::vector<std::string>, steps: std::vector<WorkflowStepSpec>) -> WorkflowBranchSpec`
- `branch(name: std::string, columns: std::initializer_list<std::string>, steps: std::initializer_list<WorkflowStepSpec>) -> WorkflowBranchSpec`
- `branch(name: std::string, columns: std::vector<std::string>, steps: std::vector<WorkflowStepSpec>) -> WorkflowBranchSpec`

## Notes

Namespace-level API function.

## Inputs

- `columns` (`std::initializer_list<std::string> | std::vector<std::string>`)
  List-like argument; pass values in the order expected by the signature.
- `name` (`std::string`)
  Method argument; see signature type for accepted form.
- `steps` (`std::initializer_list<WorkflowStepSpec> | std::vector<WorkflowStepSpec>`)
  List-like argument; pass values in the order expected by the signature.

## Output

- Returns `WorkflowBranchSpec`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = branch(/* name */, /* columns */, /* steps */);
out.info("branch", value);
```

**Expected output**

```text
branch returned WorkflowBranchSpec
```
