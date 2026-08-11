# `Result::zero_rate`

## API Signature

- `Result::zero_rate() -> const Series&`

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
auto value = Result::zero_rate();
out.info("zero_rate", value);
```

**Expected output**

```text
zero_rate returned const Series&
```
