# `grid_size`

## API Signature

- `grid_size(axes: const std::vector<GridAxis>&) -> std::int64_t`

## Notes

Namespace-level API function.

## Inputs

- `axes` (`const std::vector<GridAxis>&`)
  Input feature data used by this call.

## Output

- Returns `std::int64_t`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = grid_size(/* axes */);
out.info("grid_size", value);
```

**Expected output**

```text
grid_size: <numeric_result>
```
