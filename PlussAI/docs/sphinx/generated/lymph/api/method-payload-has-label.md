# `Payload::has_label`

## API Signature

- `Payload::has_label() -> bool`

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
auto value = Payload::has_label();
out.info("has_label", value);
```

**Expected output**

```text
has_label: true
```
