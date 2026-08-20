# `Payload::has_time`

## API Signature

- `Payload::has_time() -> bool`

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
auto value = Payload::has_time();
out.info("has_time", value);
```

**Expected output**

```text
has_time: true
```
