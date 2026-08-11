# `Result::coordinates`

## API Signature

- `Result::coordinates() -> const Series&`

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
auto value = Result::coordinates();
out.info("coordinates", value);
```

**Expected output**

```text
coordinates returned const Series&
```
