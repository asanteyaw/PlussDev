# `pluss::ai::classic::SparseTripletMatrix`

A sparse matrix in coordinate (COO) form: three parallel `Series` — one row
index, one column index, one value — per stored non-zero entry, plus the
matrix's overall shape. The easiest form to build by hand (append triples
as you discover them, in any order, duplicates and all); every sparse
learner in this family converts it to [`SparseCsrMatrix`](SparseCsrMatrix.md)
internally before it ever touches a row.

## Signature

```cpp
pluss::Series classic::SparseTripletMatrix::row_indices;
pluss::Series classic::SparseTripletMatrix::column_indices;
pluss::Series classic::SparseTripletMatrix::values;
std::int64_t classic::SparseTripletMatrix::rows;
std::int64_t classic::SparseTripletMatrix::cols;
```

## Construction

A plain aggregate — build one field-by-field, or (the common case) through
[`make_sparse_triplet`](make_sparse_triplet.md), which validates every
index and value for you before returning. Nothing stops you from
assembling a `SparseTripletMatrix` directly with out-of-range indices or
non-finite values; those are only caught once something reads it (
[`sparse_triplet_to_csr`](make_sparse_triplet.md), or a learner's own
`learn`/`infer`/`assess`, which converts internally).

## State

`row_indices` / `column_indices` — `pluss::Series`
: `Int64`, `[nnz]` — the row and column of each stored entry, in the same
  order as `values`. Entries need not be sorted, and the same
  `(row, column)` pair may appear more than once — see
  [`sparse_triplet_to_csr`](make_sparse_triplet.md#returns): conversion to
  CSR sorts entries into row-major order but does **not** merge a
  duplicate pair, both survive as separate stored entries.

`values` — `pluss::Series`
: `Float64`, `[nnz]` — the value stored at each `(row_indices[i],
  column_indices[i])` position. Every position not listed is implicitly
  `0.0`.

`rows` / `cols` — `std::int64_t`
: The matrix's overall shape — **not** inferred from the largest index
  present, so a matrix can declare trailing all-zero rows/columns no
  triplet ever touches.

## Methods

None — `SparseTripletMatrix` is a plain data holder, read directly.

## Practical Example

Four non-zero entries in a `4x3` matrix, read back field by field.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::SparseTripletMatrix triplet =
      cls::make_sparse_triplet(4, 3, {0, 0, 2, 3}, {0, 1, 0, 2}, {1.0, 2.0, 3.0, 4.0}).value();

  std::cout << "rows = " << triplet.rows << ", cols = " << triplet.cols << "\n";
  std::cout << "row_indices    = " << triplet.row_indices << "\n";
  std::cout << "column_indices = " << triplet.column_indices << "\n";
  std::cout << "values         = " << triplet.values << "\n";

  return 0;
}
```

Output

```text
rows = 4, cols = 3
row_indices    = pluss::Series(shape=[4], strides=[1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=[0, 0, 2, 3]
column_indices = pluss::Series(shape=[4], strides=[1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=[0, 1, 0, 2]
values         = pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1, 2, 3, 4]
```

## Changelog

- **0.1** — `row_indices`, `column_indices`, `values`, `rows`, `cols`.

## See Also

- [`make_sparse_triplet`](make_sparse_triplet.md) — the validating
  constructor, and `sparse_triplet_to_csr`, the conversion every learner
  runs internally.
- [`SparseCsrMatrix`](SparseCsrMatrix.md) — the row-compressed form
  learners actually compute on.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
