# `Flow::add_function_transformer`

## API Signature

- `Flow::add_function_transformer(transformer: prep::FunctionTransformer) -> Status`

## Notes

Instance method on `self`.

## Inputs

- `transformer` (`prep::FunctionTransformer`)
  Method argument; see signature type for accepted form.

## Output

- Returns a status object indicating success/failure and an error message on failure.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto st = Flow::add_function_transformer(/* transformer */);
if (!st.ok()) {
  out.error("add_function_transformer failed", st.message);
} else {
  out.info("add_function_transformer ok");
}
```

**Expected output**

```text
add_function_transformer: ok
status.ok() == true
```
