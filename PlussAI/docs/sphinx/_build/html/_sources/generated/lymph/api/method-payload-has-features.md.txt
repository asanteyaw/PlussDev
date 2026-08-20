# `Payload::has_features`

## API Signature

- `Payload::has_features() -> bool`

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
auto value = Payload::has_features();
out.info("has_features", value);
```

**Expected output**

```text
has_features: true
```
