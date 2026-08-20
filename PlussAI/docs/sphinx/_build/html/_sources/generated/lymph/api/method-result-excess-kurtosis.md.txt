# `Result::excess_kurtosis`

## API Signature

- `Result::excess_kurtosis() -> const Series&`

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
auto value = Result::excess_kurtosis();
out.info("excess_kurtosis", value);
```

**Expected output**

```text
excess_kurtosis returned const Series&
```
