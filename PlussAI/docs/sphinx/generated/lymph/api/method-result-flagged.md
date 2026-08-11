# `Result::flagged`

## API Signature

- `Result::flagged() -> Series`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `Series`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Result::flagged();
out.info("flagged", value);
```

**Expected output**

```text
flagged returned Series
```
