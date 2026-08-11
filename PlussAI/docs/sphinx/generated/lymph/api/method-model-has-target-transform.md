# `Model::has_target_transform`

## API Signature

- `Model::has_target_transform() -> bool`

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
auto value = Model::has_target_transform();
out.info("has_target_transform", value);
```

**Expected output**

```text
has_target_transform: true
```
