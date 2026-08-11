# `Payload::has_weight_feature`

## API Signature

- `Payload::has_weight_feature() -> bool`

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
auto value = Payload::has_weight_feature();
out.info("has_weight_feature", value);
```

**Expected output**

```text
has_weight_feature: true
```
