# `copy_range_values`

## API Signature

- `copy_range_values(values: RangeView::Of<T>) -> std::vector<T>`

## Notes

Namespace-level API function.

## Inputs

- `values` (`RangeView::Of<T>`)
  Method argument; see signature type for accepted form.

## Output

- Returns `std::vector<T>`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = copy_range_values(/* values */);
out.info("copy_range_values", value);
```

**Expected output**

```text
copy_range_values returned std::vector<T>
```
