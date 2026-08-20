# `Model::has_model`

## API Signature

- `Model::has_model() -> bool`

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
auto value = Model::has_model();
out.info("has_model", value);
```

**Expected output**

```text
has_model: true
```
