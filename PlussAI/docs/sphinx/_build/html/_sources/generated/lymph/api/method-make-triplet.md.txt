# `make_triplet`

## API Signature

- `make_triplet(rows: std::int64_t, cols: std::int64_t, row_indices: RangeView::Int, column_indices: RangeView::Int, values: RangeView::Of<double>) -> Triplet`
- `make_triplet(rows: std::int64_t, cols: std::int64_t, row_indices: std::initializer_list<std::int64_t>, column_indices: std::initializer_list<std::int64_t>, values: std::initializer_list<double>) -> Triplet`

## Notes

Namespace-level API function.

## Inputs

- `cols` (`std::int64_t`)
  Method argument; see signature type for accepted form.
- `column_indices` (`RangeView::Int | std::initializer_list<std::int64_t>`)
  List-like argument; pass values in the order expected by the signature.
- `row_indices` (`RangeView::Int | std::initializer_list<std::int64_t>`)
  List-like argument; pass values in the order expected by the signature.
- `rows` (`std::int64_t`)
  Method argument; see signature type for accepted form.
- `values` (`RangeView::Of<double> | std::initializer_list<double>`)
  List-like argument; pass values in the order expected by the signature.

## Output

- Returns `Triplet`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = make_triplet(/* rows */, /* cols */, /* row_indices */, /* column_indices */, /* values */);
out.info("make_triplet", value);
```

**Expected output**

```text
make_triplet returned Triplet
```
