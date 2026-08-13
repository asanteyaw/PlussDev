# `Model::target_transform`

## API Signature

- `Model::target_transform() -> const TargetTransform&`
- `Model::target_transform(transform: TargetTransform) -> Model&`

## Notes

Instance method on `self`.

## Inputs

- `transform` (`TargetTransform`)
  Method argument; see signature type for accepted form.

## Output

- Returns `Model&`.
- Returns `const TargetTransform&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Model::target_transform();
out.info("target_transform", value);
```

**Expected output**

```text
target_transform returned const TargetTransform&
```
