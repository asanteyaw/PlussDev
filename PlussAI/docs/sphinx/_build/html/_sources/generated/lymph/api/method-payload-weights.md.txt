# `Payload::weights`

## API Signature

- `Payload::weights() -> const Series&`
- `Payload::weights(sample_weights: Series) -> Payload&`
- `Payload::weights(feature_name: std::string) -> Payload&`

## Notes

Instance method on `self`.

## Inputs

- `feature_name` (`std::string`)
  Input feature data used by this call.
- `sample_weights` (`Series`)
  Optional per-row weights aligned to input observations.

## Output

- Returns `Payload&`.
- Returns `const Series&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Payload::weights();
out.info("weights", value);
```

**Expected output**

```text
weights returned const Series&
```
