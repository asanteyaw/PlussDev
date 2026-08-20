# `Result::variances`

## API Signature

- `Result::variances() -> Series`

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
auto value = Result::variances();
out.info("variances", value);
```

**Expected output**

```text
variances returned Series
```
