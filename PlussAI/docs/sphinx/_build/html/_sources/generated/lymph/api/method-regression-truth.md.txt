# `Regression::truth`

## API Signature

- `Regression::truth() -> const Series&`

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
auto value = Regression::truth();
out.info("truth", value);
```

**Expected output**

```text
truth returned const Series&
```
