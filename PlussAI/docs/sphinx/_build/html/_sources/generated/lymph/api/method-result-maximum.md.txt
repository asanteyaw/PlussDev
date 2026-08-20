# `Result::maximum`

## API Signature

- `Result::maximum() -> const Series&`

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
auto value = Result::maximum();
out.info("maximum", value);
```

**Expected output**

```text
maximum returned const Series&
```
