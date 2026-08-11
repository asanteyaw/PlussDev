# `Payload::with_frame`

## API Signature

- `Payload::with_frame(frame: pluss::table::DataFrame) -> Payload`

## Notes

Instance method on `self`.

## Inputs

- `frame` (`pluss::table::DataFrame`)
  Input feature data used by this call.

## Output

- Returns `Payload`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Payload::with_frame(/* frame */);
out.info("with_frame", value);
```

**Expected output**

```text
with_frame returned Payload
```
