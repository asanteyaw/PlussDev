# `WorkflowBranch::route`

## API Signature

- `WorkflowBranch::route(router: ColumnRouter) -> WorkflowRoute`
- `WorkflowBranch::route(predicate: std::function<bool(std::string_view feature_name, std::string_view dtype)>) -> WorkflowRoute`
- `WorkflowBranch::route(names: std::initializer_list<std::string>) -> WorkflowRoute`
- `WorkflowBranch::route(names: std::vector<std::string>) -> WorkflowRoute`

## Notes

Instance method on `self`.

## Inputs

- `names` (`std::initializer_list<std::string> | std::vector<std::string>`)
  List-like argument; pass values in the order expected by the signature.
- `predicate` (`std::function<bool(std::string_view feature_name, std::string_view dtype)>`)
  Method argument; see signature type for accepted form.
- `router` (`ColumnRouter`)
  Method argument; see signature type for accepted form.

## Output

- Returns `WorkflowRoute`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = WorkflowBranch::route(/* router */);
out.info("route", value);
```

**Expected output**

```text
route returned WorkflowRoute
```
