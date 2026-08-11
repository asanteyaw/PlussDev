# `Result::negative_rate`

## API Signature

- `Result::negative_rate() -> const Series&`

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
auto value = Result::negative_rate();
out.info("negative_rate", value);
```

**Expected output**

```text
negative_rate returned const Series&
```
