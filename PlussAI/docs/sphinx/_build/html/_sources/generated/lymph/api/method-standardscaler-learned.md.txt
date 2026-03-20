# `StandardScaler::learned`

## API Signature

- `StandardScaler::learned() -> bool`

## Notes

Fits model/transform state from the provided training data. Call this before inference-oriented methods.

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
auto value = StandardScaler::learned();
out.info("learned", value);
```

**Expected output**

```text
learned: true
```
