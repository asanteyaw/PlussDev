# `Result::labeled_round`

## API Signature

- `Result::labeled_round() -> const Series&`

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
auto value = Result::labeled_round();
out.info("labeled_round", value);
```

**Expected output**

```text
labeled_round returned const Series&
```
