# `Model::transform_specs`

## API Signature

- `Model::transform_specs() -> const std::vector<TransformSpec>&`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `const std::vector<TransformSpec>&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Model::transform_specs();
out.info("transform_specs", value);
```

**Expected output**

```text
transform_specs returned const std::vector<TransformSpec>&
```
