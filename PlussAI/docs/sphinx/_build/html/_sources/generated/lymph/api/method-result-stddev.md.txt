# `Result::stddev`

## API Signature

- `Result::stddev() -> const Series&`

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
auto value = Result::stddev();
out.info("stddev", value);
```

**Expected output**

```text
stddev returned const Series&
```
