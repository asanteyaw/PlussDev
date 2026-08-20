# `Result::skewness`

## API Signature

- `Result::skewness() -> const Series&`

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
auto value = Result::skewness();
out.info("skewness", value);
```

**Expected output**

```text
skewness returned const Series&
```
