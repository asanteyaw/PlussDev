# `ModelCore::nested_param_keys`

## API Signature

- `ModelCore::nested_param_keys() -> std::vector<std::string>`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `std::vector<std::string>`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = ModelCore::nested_param_keys();
out.info("nested_param_keys", value);
```

**Expected output**

```text
nested_param_keys returned std::vector<std::string>
```
