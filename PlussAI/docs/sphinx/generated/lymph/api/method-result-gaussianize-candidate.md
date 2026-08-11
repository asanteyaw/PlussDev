# `Result::gaussianize_candidate`

## API Signature

- `Result::gaussianize_candidate() -> const Series&`

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
auto value = Result::gaussianize_candidate();
out.info("gaussianize_candidate", value);
```

**Expected output**

```text
gaussianize_candidate returned const Series&
```
