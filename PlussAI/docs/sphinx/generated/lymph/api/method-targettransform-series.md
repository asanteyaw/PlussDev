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

No explicit option struct/enum parameters were detected for this method.

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
