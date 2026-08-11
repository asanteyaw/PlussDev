# `Workflow::on`

## API Signature

- `Workflow::on(router: ColumnRouter) -> WorkflowRoute`
- `Workflow::on(predicate: std::function<bool(std::string_view feature_name, std::string_view dtype)>) -> WorkflowRoute`
- `Workflow::on(names: std::initializer_list<std::string>) -> WorkflowRoute`
- `Workflow::on(name: std::string, router: ColumnRouter) -> WorkflowRoute`
- `Workflow::on(name: std::string, predicate: std::function<bool(std::string_view feature_name, std::string_view dtype)>) -> WorkflowRoute`
- `Workflow::on(name: std::string, names: std::initializer_list<std::string>) -> WorkflowRoute`
- `Workflow::on(name: std::string, names: std::vector<std::string>) -> WorkflowRoute`
- `Workflow::on(names: std::vector<std::string>) -> WorkflowRoute`
- `Workflow::on(router: ColumnRouter) -> WorkflowRoute`
- `Workflow::on(predicate: std::function<bool(std::string_view feature_name, std::string_view dtype)>) -> WorkflowRoute`
- `Workflow::on(names: std::initializer_list<std::string>) -> WorkflowRoute`
- `Workflow::on(name: std::string, router: ColumnRouter) -> WorkflowRoute`
- `Workflow::on(name: std::string, predicate: std::function<bool(std::string_view feature_name, std::string_view dtype)>) -> WorkflowRoute`
- `Workflow::on(name: std::string, names: std::initializer_list<std::string>) -> WorkflowRoute`
- `Workflow::on(name: std::string, names: std::vector<std::string>) -> WorkflowRoute`
- `Workflow::on(names: std::vector<std::string>) -> WorkflowRoute`

## Notes

Instance method on `self`.

## Inputs

- `name` (`std::string`)
  Method argument; see signature type for accepted form.
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
auto value = Workflow::on(/* router */);
out.info("on", value);
```

**Expected output**

```text
on returned WorkflowRoute
```

### Example 2 (Demo Pattern)

```cpp
      {"weathersit", "weather_condition"},
      {"hum", "humidity"},
      {"mnth", "month"},
      {"cnt", "total_count"},
      {"hr", "hour"},
      {"yr", "year"},
  });

  // Selection belongs on features(); cast stays a pure transform surface.
  frame.set("date") = frame.features("date").cast().timeline();
  frame = frame.astype({
      {"season", ScalarType::Float64},
      {"year", ScalarType::Float64},
      {"month", ScalarType::Float64},
      {"hour", ScalarType::Float64},
      {"is_holiday", ScalarType::Float64},
      {"weekday", ScalarType::Float64},
      {"is_workingday", ScalarType::Float64},
      {"weather_condition", ScalarType::Float64},
      {"temp", ScalarType::Float64},
      {"atemp", ScalarType::Float64},
```

**Expected output**

```text
on returned WorkflowRoute
```
