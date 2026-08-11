# `Workflow::has_opaque_transforms`

## API Signature

- `Workflow::has_opaque_transforms() -> bool`

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
auto value = Workflow::has_opaque_transforms();
out.info("has_opaque_transforms", value);
```

**Expected output**

```text
has_opaque_transforms: true
```
