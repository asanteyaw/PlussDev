# `ModelReports::ModelReports`

## API Signature

- `ModelReports(arg1: Model model) : model_(std::move(model)) -> auto`

## Notes

Instance method on `self`.

## Inputs

- `arg1` (`Model model) : model_(std::move(model)`)
  Method argument; see signature type for accepted form.

## Output

- Returns method result for the selected overload.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = ModelReports(/* arg1 */);
out.info("ModelReports", value);
```

**Expected output**

```text
ModelReports returned result
```
