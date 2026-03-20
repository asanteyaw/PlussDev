# `Workflow::model_spec`

## API Signature

- `Workflow::model_spec() -> const std::optional<ModelSpec>&`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `const std::optional<ModelSpec>&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Workflow::model_spec();
out.info("model_spec", value);
```

**Expected output**

```text
model_spec returned const std::optional<ModelSpec>&
```
