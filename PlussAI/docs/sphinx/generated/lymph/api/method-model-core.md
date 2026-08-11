# `model_core`

## API Signature

- `model_core(model: const Model&) -> const ModelCore&`
- `model_core(model: const Model&) -> const ModelCore&`

## Notes

Namespace-level API function.

## Inputs

- `model` (`const Model&`)
  Method argument; see signature type for accepted form.

## Output

- Returns `const ModelCore&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = model_core(/* model */);
out.info("model_core", value);
```

**Expected output**

```text
model_core returned const ModelCore&
```
