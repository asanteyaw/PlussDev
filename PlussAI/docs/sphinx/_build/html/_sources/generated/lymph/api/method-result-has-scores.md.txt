# `Result::has_scores`

## API Signature

- `Result::has_scores() -> bool`

## Notes

Boolean introspection helper that reports whether a capability/state is available.

## Inputs

None.

## Output

- Returns `bool`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Result::has_scores();
out.info("has_scores", value);
```

**Expected output**

```text
has_scores: true
```
