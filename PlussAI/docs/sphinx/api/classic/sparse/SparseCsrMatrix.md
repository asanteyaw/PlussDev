# `pluss::ai::classic::SparseCsrMatrix`

A sparse matrix in compressed-sparse-row (CSR) form — the layout
[`SparseLinearBinaryClassifier`](SparseLinearBinaryClassifier.md) /
[`SparseLinearRegressor`](SparseLinearRegressor.md) /
[`SparseLinearMultiClassifier`](SparseLinearMultiClassifier.md) actually
compute on; a [`SparseTripletMatrix`](SparseTripletMatrix.md) is converted
to this form internally before every `learn`/`infer`/`assess` call. Unlike
almost every other data type in this reference, its fields are plain
`std::vector`s, not `pluss::Series` — this is a CPU-only, host-side
structure (the sparse learners are CPU-only solvers), not a
platform-portable tensor.

## Signature

```cpp
std::vector<std::int64_t> classic::SparseCsrMatrix::row_ptr;
std::vector<std::int64_t> classic::SparseCsrMatrix::col_ind;
std::vector<double> classic::SparseCsrMatrix::values;
std::int64_t classic::SparseCsrMatrix::rows;
std::int64_t classic::SparseCsrMatrix::cols;
```

## Construction

A plain aggregate. Build one directly only if you already have data in
this exact layout; the ordinary path is
[`sparse_triplet_to_csr`](make_sparse_triplet.md), which builds one from a
[`SparseTripletMatrix`](SparseTripletMatrix.md) by sorting its entries into
row-major order — it does **not** merge a duplicate `(row, column)` pair
into one entry; see [State](#state) below.

## State

`row_ptr` — `std::vector<std::int64_t>`
: Length `rows + 1`. Row `r`'s entries occupy `col_ind`/`values` positions
  `[row_ptr[r], row_ptr[r + 1])` — the standard CSR offset array.
  `row_ptr[0]` is always `0`; `row_ptr[rows]` is the total non-zero count.

`col_ind` / `values` — `std::vector<std::int64_t>` / `std::vector<double>`
: Length equal to the source triplet's own entry count — converting to CSR
  never drops or merges anything. `col_ind[p]` / `values[p]` give the
  column and value of the `p`-th stored entry; within one row's slice,
  entries are sorted by column ascending, and
  [`sparse_triplet_to_csr`](make_sparse_triplet.md) uses a **stable** sort,
  so two entries sharing the same `(row, column)` keep their original
  relative order and both appear here, back to back, as separate stored
  entries — **not** merged or summed into one. See
  [Example 1](#practical-example) for exactly this case. This is harmless
  for every estimator in this family: each one's own scoring pass sums
  every stored entry in a row regardless of repeated columns
  (`bias + sum_p weights[col_ind[p]] * values[p]`), which is arithmetically
  identical to having summed the duplicate first — but any other code
  reading `col_ind`/`values` directly should not assume one entry per
  `(row, column)` pair.

`rows` / `cols` — `std::int64_t`
: The matrix's overall shape, carried over unchanged from the source
  [`SparseTripletMatrix`](SparseTripletMatrix.md).

## Methods

None — `SparseCsrMatrix` is a plain data holder, read directly.

## Practical Example

The same `4x3` matrix as [`SparseTripletMatrix`'s own
example](SparseTripletMatrix.md), including one duplicate `(row, column)`
pair, converted to CSR — both duplicate entries survive as separate stored
entries in row 0's slice, in their original relative order, and each
row's own slice of `col_ind`/`values` is read out via `row_ptr`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  // row 0 gets column 0 twice: 1.0 then 1.5 -- both survive as separate
  // stored entries, not merged into 2.5.
  const cls::SparseTripletMatrix triplet =
      cls::make_sparse_triplet(4, 3, {0, 0, 0, 2, 3}, {0, 0, 1, 0, 2}, {1.0, 1.5, 2.0, 3.0, 4.0}).value();
  const cls::SparseCsrMatrix csr = cls::sparse_triplet_to_csr(triplet).value();

  std::cout << "row_ptr = [";
  for (std::int64_t v : csr.row_ptr) std::cout << v << " ";
  std::cout << "]\n";

  for (std::int64_t r = 0; r < csr.rows; ++r) {
    std::cout << "row " << r << ": ";
    for (std::int64_t p = csr.row_ptr[static_cast<std::size_t>(r)]; p < csr.row_ptr[static_cast<std::size_t>(r + 1)]; ++p) {
      std::cout << "(col=" << csr.col_ind[static_cast<std::size_t>(p)]
                << ", val=" << csr.values[static_cast<std::size_t>(p)] << ") ";
    }
    std::cout << "\n";
  }

  return 0;
}
```

Output

```text
row_ptr = [0 3 3 4 5 ]
row 0: (col=0, val=1) (col=0, val=1.5) (col=1, val=2) 
row 1: 
row 2: (col=0, val=3) 
row 3: (col=2, val=4) 
```

## Changelog

- **0.1** — `row_ptr`, `col_ind`, `values`, `rows`, `cols`.

## See Also

- [`SparseTripletMatrix`](SparseTripletMatrix.md) — the coordinate form
  this is converted from.
- [`make_sparse_triplet`](make_sparse_triplet.md) — `sparse_triplet_to_csr`,
  the producing function.
- [`SparseLinearBinaryClassifier`](SparseLinearBinaryClassifier.md) /
  [`SparseLinearRegressor`](SparseLinearRegressor.md) /
  [`SparseLinearMultiClassifier`](SparseLinearMultiClassifier.md) — every
  `learn`/`infer`/`assess` overload that accepts this form directly (and
  saves the internal conversion a `SparseTripletMatrix` call would pay).

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
