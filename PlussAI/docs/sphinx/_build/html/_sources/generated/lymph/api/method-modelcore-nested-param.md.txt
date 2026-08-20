# `ModelCore::nested_param`

## API Signature

- `ModelCore::nested_param(path: std::string_view) -> double`

## Notes

Instance method on `self`.

## Inputs

- `path` (`std::string_view`)
  Path to persisted model/data artifact.

## Output

- Returns `double`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = ModelCore::nested_param(/* path */);
out.info("nested_param", value);
```

**Expected output**

```text
nested_param: <numeric_result>
```
