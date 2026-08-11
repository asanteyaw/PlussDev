# `Result::minimum`

## API Signature

- `Result::minimum() -> const Series&`

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
auto value = Result::minimum();
out.info("minimum", value);
```

**Expected output**

```text
minimum returned const Series&
```
