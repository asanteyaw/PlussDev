# `custom`

## API Signature

- `custom(name: std::string, forward: std::function<Series(const Series&)>, inverse: std::function<Series(const Series&)>, check_inverse: bool = true, check_inverse_tolerance: double = 1e-3) -> TargetTransform`

## Notes

Namespace-level API function.

## Inputs

- `check_inverse` (`bool`), default: `true`
  Algorithm-size/count argument used to shape fit or split behavior.
- `check_inverse_tolerance` (`double`), default: `1e-3`
  Algorithm-size/count argument used to shape fit or split behavior.
- `forward` (`std::function<Series(const Series&)>`)
  Pluss numeric vector/tensor input.
- `inverse` (`std::function<Series(const Series&)>`)
  Pluss numeric vector/tensor input.
- `name` (`std::string`)
  Method argument; see signature type for accepted form.

## Output

- Returns `TargetTransform`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = custom(/* name */, /* forward */, /* inverse */, /* check_inverse */, /* check_inverse_tolerance */);
out.info("custom", value);
```

**Expected output**

```text
custom returned TargetTransform
```
