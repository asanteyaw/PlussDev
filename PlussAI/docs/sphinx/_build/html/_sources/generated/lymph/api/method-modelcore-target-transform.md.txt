# `ModelCore::target_transform`

## API Signature

- `ModelCore::target_transform() -> const TargetTransform&`
- `ModelCore::target_transform(transform: TargetTransform) -> ModelCore&`

## Notes

Instance method on `self`.

## Inputs

- `transform` (`TargetTransform`)
  Method argument; see signature type for accepted form.

## Output

- Returns `ModelCore&`.
- Returns `const TargetTransform&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = ModelCore::target_transform();
out.info("target_transform", value);
```

**Expected output**

```text
target_transform returned const TargetTransform&
```
