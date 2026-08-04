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

### Example 2 (Demo Pattern)

```cpp
    return 1;
  }
  std::cout << "dense objective: " << tune_or.value().ledger.objective << "\n";
  std::cout << "dense ranks: " << tune_or.value().ledger.objective_rank << "\n";
  std::cout << "dense encoded params shape: "
            << tune_or.value().ledger.encoded_params.sizes()[0] << "x"
            << tune_or.value().ledger.encoded_params.sizes()[1] << "\n";

  const auto sparse_or = make_sparse_triplet(
      6,
      3,
      {0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 5},
      {0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1},
      {-2.0, -1.0, 0.0, 1.0, 2.0, 3.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0});
  if (!sparse_or.ok()) {
    std::cerr << sparse_or.status().message << "\n";
    return 1;
  }
  const std::vector<LinearFamilyOptions> sparse_candidates{
      LinearFamilyOptions{
          .objective = LinearObjective::Logistic,
```

**Expected output**

```text
class scores: pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 13.5, 0]
class rank: pluss::Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[1, 0, 2]
picked feature ids: pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[1, 0]
picked shape: 6x2
regression picked ids: pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[1, 2]
dense objective: pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.66666667, 0.5]
dense ranks: pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
...
```
