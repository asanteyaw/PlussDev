# `TargetTransform::Series`

## API Signature

- `TargetTransform::Series(arg2: const Series&) -> std::function<`
- `TargetTransform::Series(arg2: const Series&) -> std::function<`

## Notes

Instance method on `self`.

## Inputs

- `arg2` (`const Series&`)
  Pluss numeric vector/tensor input.

## Output

- Returns `std::function<`.

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
auto value = TargetTransform::Series(/* arg2 */);
out.info("Series", value);
```

**Expected output**

```text
Series returned std::function<
```
