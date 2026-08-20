# `Model::function_transformer`

## API Signature

- `Model::function_transformer(transformer: prep::FunctionTransformer) -> Model&`

## Notes

Instance method on `self`.

## Inputs

- `transformer` (`prep::FunctionTransformer`)
  Method argument; see signature type for accepted form.

## Output

- Returns `Model&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Model::function_transformer(/* transformer */);
out.info("function_transformer", value);
```

**Expected output**

```text
function_transformer returned Model&
```
