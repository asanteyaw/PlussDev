# `Payload::has_external_weights`

## API Signature

- `Payload::has_external_weights() -> bool`

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
auto value = Payload::has_external_weights();
out.info("has_external_weights", value);
```

**Expected output**

```text
has_external_weights: true
```
