# `grid_points`

## API Signature

- `grid_points(axes: const std::vector<GridAxis>&) -> std::vector<GridPoint>`

## Notes

Namespace-level API function.

## Inputs

- `axes` (`const std::vector<GridAxis>&`)
  Input feature data used by this call.

## Output

- Returns `std::vector<GridPoint>`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = grid_points(/* axes */);
out.info("grid_points", value);
```

**Expected output**

```text
grid_points returned std::vector<GridPoint>
```
