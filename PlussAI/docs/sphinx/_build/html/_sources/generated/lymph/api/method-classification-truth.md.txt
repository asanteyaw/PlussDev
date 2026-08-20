# `Classification::truth`

## API Signature

- `Classification::truth() -> const Series&`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `const Series&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Classification::truth();
out.info("truth", value);
```

**Expected output**

```text
truth returned const Series&
```
