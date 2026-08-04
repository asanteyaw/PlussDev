# `copy_feature_route`

## API Signature

- `copy_feature_route(feature_route: RangeView::Int) -> std::vector<std::int64_t>`

## Notes

Namespace-level API function.

## Inputs

- `feature_route` (`RangeView::Int`)
  Input feature data used by this call.

## Output

- Returns `std::vector<std::int64_t>`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = copy_feature_route(/* feature_route */);
out.info("copy_feature_route", value);
```

**Expected output**

```text
copy_feature_route returned std::vector<std::int64_t>
```
