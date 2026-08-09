# `grid_point_at`

## API Signature

- `grid_point_at(index: std::int64_t, axes: const std::vector<GridAxis>&) -> GridPoint`

## Notes

Namespace-level API function.

## Inputs

- `axes` (`const std::vector<GridAxis>&`)
  Input feature data used by this call.
- `index` (`std::int64_t`)
  Input feature data used by this call.

## Output

- Returns `GridPoint`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = grid_point_at(/* index */, /* axes */);
out.info("grid_point_at", value);
```

**Expected output**

```text
grid_point_at returned GridPoint
```
