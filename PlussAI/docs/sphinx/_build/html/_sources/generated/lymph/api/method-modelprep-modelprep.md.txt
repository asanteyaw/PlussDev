# `ModelPrep::ModelPrep`

## API Signature

- `ModelPrep(model: Model& model) noexcept : model_(&) -> auto`

## Notes

Instance method on `self`.

## Inputs

- `model` (`Model& model) noexcept : model_(&`)
  Method argument; see signature type for accepted form.

## Output

- Returns method result for the selected overload.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = ModelPrep(/* model */);
out.info("ModelPrep", value);
```

**Expected output**

```text
ModelPrep returned result
```
