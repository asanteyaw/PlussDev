# `make_sparse_triplet`

## API Signature

- `make_sparse_triplet(rows: std::int64_t, cols: std::int64_t, row_indices: std::initializer_list<std::int64_t>, column_indices: std::initializer_list<std::int64_t>, values: std::initializer_list<double>) -> StatusOr<SparseTripletMatrix>`
- `make_sparse_triplet(rows: std::int64_t, cols: std::int64_t, row_indices: RangeView::Int, column_indices: RangeView::Int, values: RangeView::Of<double>) -> StatusOr<SparseTripletMatrix>`

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

- Returns `StatusOr<SparseTripletMatrix>`. Check `.ok()` first; on success read `.value()` for the computed result.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto res = make_sparse_triplet(/* rows */, /* cols */, /* row_indices */, /* column_indices */, /* values */);
if (!res.ok()) {
  out.error("make_sparse_triplet failed", res.status().message);
} else {
  out.info("make_sparse_triplet ok");
}
```

**Expected output**

```text
make_sparse_triplet: ok
status.ok() == true
result.value() is available
```
