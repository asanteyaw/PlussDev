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

Option semantics used by this method:

- `TargetTransform` (`struct`):
  - `std::string name = "target_transform"`
    Option field used by this method.
  - `bool check_inverse = true`
    Controls fold/trial/count sizing.
  - `double check_inverse_tolerance = 1e-3`
    Controls fold/trial/count sizing.

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
