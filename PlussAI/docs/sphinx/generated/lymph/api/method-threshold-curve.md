# `Threshold::curve`

## API Signature

- `Threshold::curve() -> ThresholdCurve`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `ThresholdCurve`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Threshold::curve();
out.info("curve", value);
```

**Expected output**

```text
curve returned ThresholdCurve
```
