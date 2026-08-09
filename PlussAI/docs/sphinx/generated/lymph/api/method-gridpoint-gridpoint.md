# `GridPoint::GridPoint`

## API Signature

- `axes_(arg1: &axes), arg2: values_(std::move(values)) -> GridPoint(const std::vector<GridAxis>& axes, std::vector<double> values) :`

## Notes

Instance method on `self`.

## Inputs

- `arg1` (`&axes)`)
  Method argument; see signature type for accepted form.
- `arg2` (`values_(std::move(values)`)
  Method argument; see signature type for accepted form.

## Output

- Returns `GridPoint(const std::vector<GridAxis>& axes, std::vector<double> values) :`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = axes_(/* arg1 */, /* arg2 */);
out.info("GridPoint", value);
```

**Expected output**

```text
GridPoint returned GridPoint(const std::vector<GridAxis>& axes, std::vector<double> values) :
```
