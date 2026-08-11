# `Workflow::has_model_spec`

## API Signature

- `Workflow::has_model_spec() -> bool`

## Notes

Boolean introspection helper that reports whether a capability/state is available.

## Inputs

None.

## Output

- Returns `bool`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Workflow::has_model_spec();
out.info("has_model_spec", value);
```

**Expected output**

```text
has_model_spec: true
```
