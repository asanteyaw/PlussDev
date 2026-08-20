# `Model::linear_parameters`

## API Signature

- `Model::linear_parameters() -> LinearParameters`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `LinearParameters`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Model::linear_parameters();
out.info("linear_parameters", value);
```

**Expected output**

```text
linear_parameters returned LinearParameters
```
