# `ModelCore::function_transformer`

## API Signature

- `ModelCore::function_transformer(transformer: prep::FunctionTransformer) -> ModelCore&`

## Notes

Instance method on `self`.

## Inputs

- `transformer` (`prep::FunctionTransformer`)
  Method argument; see signature type for accepted form.

## Output

- Returns `ModelCore&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = ModelCore::function_transformer(/* transformer */);
out.info("function_transformer", value);
```

**Expected output**

```text
function_transformer returned ModelCore&
```
