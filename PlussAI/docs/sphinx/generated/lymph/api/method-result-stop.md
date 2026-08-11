# `Result::stop`

## API Signature

- `Result::stop() -> StopReason`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `StopReason`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Result::stop();
out.info("stop", value);
```

**Expected output**

```text
stop returned StopReason
```
