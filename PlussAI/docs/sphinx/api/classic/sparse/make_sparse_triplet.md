# `pluss::ai::classic::make_sparse_triplet` / `sparse_triplet_to_csr`

`make_sparse_triplet` validates and builds a
[`SparseTripletMatrix`](SparseTripletMatrix.md) from parallel row/column/
value ranges — every index checked in range, every value checked finite,
before anything is returned. `sparse_triplet_to_csr` converts one to
[`SparseCsrMatrix`](SparseCsrMatrix.md), the form every sparse learner
actually computes on — every `learn`/`infer`/`assess` overload that takes
a `SparseTripletMatrix` runs this same conversion internally, so calling
it yourself first only matters when you plan to reuse the same matrix
across several calls and want to pay the conversion cost once.

## Signature

```cpp
classic::StatusOr<classic::SparseTripletMatrix> classic::make_sparse_triplet(
    std::int64_t rows,
    std::int64_t cols,
    /* a range of int64_t */ row_indices,
    /* a range of int64_t */ column_indices,
    /* a range of double */ values);

classic::StatusOr<classic::SparseTripletMatrix> classic::make_sparse_triplet(
    std::int64_t rows,
    std::int64_t cols,
    std::initializer_list<std::int64_t> row_indices,
    std::initializer_list<std::int64_t> column_indices,
    std::initializer_list<double> values);

classic::StatusOr<classic::SparseCsrMatrix> classic::sparse_triplet_to_csr(
    const classic::SparseTripletMatrix& sparse);
```

The second `make_sparse_triplet` overload is the same call spelled with
three braced `{...}` lists instead of three existing ranges (a
`std::vector<std::int64_t>`, for instance) — the common case when the
triples are known at the call site rather than already collected
somewhere else.

## What It Does

`make_sparse_triplet` validates every `(row, column, value)` triple --
indices in range, values finite -- and bundles the three parallel arrays
into one `SparseTripletMatrix`, rejecting anything invalid before
returning rather than deferring the check to first use.
`sparse_triplet_to_csr` converts an already-built triplet matrix into the
compressed-row form every sparse learner's `learn`/`infer`/`assess` code
path actually runs against internally.

## Parameters

`rows` / `cols`
: The matrix's declared shape. Both must be `> 0`.

`row_indices` / `column_indices` / `values`
: One entry per non-zero triple, all three the same length. Every
  `row_indices[i]` must be in `[0, rows)`, every `column_indices[i]` in
  `[0, cols)`, and every `values[i]` finite. Order does not matter, and
  the same `(row, column)` pair may repeat — see `sparse_triplet_to_csr`
  below for how a repeat is resolved.

`sparse` (`sparse_triplet_to_csr` only)
: The triplet matrix to convert. Must have `rows > 0`, `cols > 0`, and
  three defined, equal-length tensors — the same shape
  [`make_sparse_triplet`](#parameters) itself already guarantees, so
  converting a matrix it returned never fails on this account.

## Returns

`make_sparse_triplet` returns a `StatusOr<SparseTripletMatrix>` — see
[`SparseTripletMatrix`](SparseTripletMatrix.md) for its fields.

`sparse_triplet_to_csr` returns a `StatusOr<SparseCsrMatrix>` — see
[`SparseCsrMatrix`](SparseCsrMatrix.md). Entries are stably sorted by row,
then by column within each row; the entry count never changes — a repeated
`(row, column)` pair from the source triplet is **not** merged, both
entries survive as separate stored entries in their original relative
order (see [Example 3](#example-3)). This is harmless for every sparse
estimator's own use of the result (each one's scoring pass sums every
stored entry in a row, so two entries sharing a column contribute the same
total as one pre-summed entry would), but direct readers of
`SparseCsrMatrix::col_ind`/`values` should not assume one entry per
`(row, column)` pair.

## Errors

- `rows <= 0` or `cols <= 0` — `Status::invalid_argument`.
- `row_indices`/`column_indices`/`values` have unequal lengths —
  `Status::shape_mismatch`.
- Any row/column index outside its declared range —
  `Status::shape_mismatch`.
- Any value not finite (`NaN`/`Inf`) — `Status::invalid_argument`.
- `sparse_triplet_to_csr`: `sparse.rows <= 0`/`cols <= 0` —
  `Status::shape_mismatch`; an undefined triplet tensor —
  `Status::invalid_argument`; unequal tensor lengths —
  `Status::shape_mismatch`.

## Practical Examples

The range-based overload building from `std::vector`s already in hand,
the braced-list overload for triples known at the call site, a duplicate
`(row, column)` pair surviving CSR conversion as two separate entries
rather than being merged, and the two most distinct rejections — an
out-of-range index and a non-finite value.

### Example 1

Building from three `std::vector`s already collected elsewhere — the
range-based overload.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>
#include <vector>

int main() {
  namespace cls = pluss::ai::classic;

  const std::vector<std::int64_t> rows_v = {0, 1, 2};
  const std::vector<std::int64_t> cols_v = {0, 1, 2};
  const std::vector<double> vals_v = {1.0, 2.0, 3.0};

  const cls::StatusOr<cls::SparseTripletMatrix> triplet =
      cls::make_sparse_triplet(3, 3, rows_v, cols_v, vals_v);
  std::cout << "ok = " << std::boolalpha << triplet.ok() << "\n";
  std::cout << "values = " << triplet.value().values << "\n";

  return 0;
}
```

Output

```text
ok = true
values = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 2, 3]
```

### Example 2

The braced-list overload — the common case, triples spelled directly.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::SparseTripletMatrix triplet =
      cls::make_sparse_triplet(4, 3, {0, 0, 2, 3}, {0, 1, 0, 2}, {1.0, 2.0, 3.0, 4.0}).value();
  std::cout << "rows = " << triplet.rows << ", cols = " << triplet.cols << "\n";

  return 0;
}
```

Output

```text
rows = 4, cols = 3
```

### Example 3

A duplicate `(row, column)` pair — `sparse_triplet_to_csr` keeps **both**
entries rather than merging them: `nnz` stays at the input count of `3`,
and the two `(0, 0)` values appear back to back, in their original
relative order (`1.0` before `1.5`, not summed into `2.5`).

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  // (0, 0) is listed twice: 1.0 and 1.5 -- both survive as separate
  // stored entries in the CSR result.
  const cls::SparseTripletMatrix triplet =
      cls::make_sparse_triplet(2, 2, {0, 0, 1}, {0, 0, 1}, {1.0, 1.5, 2.0}).value();
  const cls::SparseCsrMatrix csr = cls::sparse_triplet_to_csr(triplet).value();

  std::cout << "nnz stored (still 3, nothing merged) = " << csr.values.size() << "\n";
  std::cout << "(0,0) first stored value  = " << csr.values[0] << "\n";
  std::cout << "(0,0) second stored value = " << csr.values[1] << "\n";

  return 0;
}
```

Output

```text
nnz stored (still 3, nothing merged) = 3
(0,0) first stored value  = 1
(0,0) second stored value = 1.5
```

### Example 4

An out-of-range column index is rejected before any `Series` is built.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::StatusOr<cls::SparseTripletMatrix> triplet =
      cls::make_sparse_triplet(2, 2, {0}, {5}, {1.0});  // column 5 is outside [0, 2)
  std::cout << "ok = " << std::boolalpha << triplet.ok() << "\n";
  std::cout << "message = \"" << triplet.status().message << "\"\n";

  return 0;
}
```

Output

```text
ok = false
message = "make_sparse_triplet encountered out-of-range row/column index."
```

### Example 5

A non-finite value is rejected the same way.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>
#include <limits>

int main() {
  namespace cls = pluss::ai::classic;

  const double nan = std::numeric_limits<double>::quiet_NaN();
  const cls::StatusOr<cls::SparseTripletMatrix> triplet =
      cls::make_sparse_triplet(2, 2, {0}, {0}, {nan});
  std::cout << "ok = " << std::boolalpha << triplet.ok() << "\n";
  std::cout << "message = \"" << triplet.status().message << "\"\n";

  return 0;
}
```

Output

```text
ok = false
message = "make_sparse_triplet requires finite values."
```

## Changelog

- **0.1** — `make_sparse_triplet` (range and braced-list overloads),
  `sparse_triplet_to_csr`.

## See Also

- [`SparseTripletMatrix`](SparseTripletMatrix.md) / [`SparseCsrMatrix`](SparseCsrMatrix.md) —
  the two types involved.
- [`SparseLinearBinaryClassifier`](SparseLinearBinaryClassifier.md) /
  [`SparseLinearRegressor`](SparseLinearRegressor.md) /
  [`SparseLinearMultiClassifier`](SparseLinearMultiClassifier.md) — every
  `learn`/`infer`/`assess` overload that runs this same conversion
  internally.
