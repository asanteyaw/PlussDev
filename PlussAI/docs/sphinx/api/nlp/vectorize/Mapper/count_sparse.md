<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: b4d032cc85b0 -->
# `pluss::ai::nlp::vectorize::Mapper::count_sparse`

[Back to `pluss::ai::nlp::vectorize::Mapper`](../Mapper.md)

## Signature

```cpp
pluss::ai::classic::StatusOr<pluss::ai::classic::SparseTripletMatrix>
pluss::ai::nlp::vectorize::Mapper::count_sparse(const std::vector<pluss::ai::nlp::Doc>& docs) const;
```

## What It Does

Maps documents to the same counts as [`count_dense`](count_dense.md), but stores only the nonzero
entries. The result is a triplet matrix: three parallel series of row indices, column indices, and
values, plus the matrix's row and column counts. Each triplet says that document `row` contains
vocabulary term `column` `value` times.

Sparse storage is the right form for a corpus where most documents use a small part of the
vocabulary, which is the normal case for text. The entries describe exactly the same counts as the
dense matrix, so either form can feed the same model.

## Parameters

`docs` is the documents to map, in the form the pipeline produces. Row indices run from zero to the
number of documents minus one, and a document with no vocabulary terms contributes no triplets.

## Returns

A classic `StatusOr<SparseTripletMatrix>`. Its `rows` field is the number of documents, `cols` is the
vocabulary size, and `row_indices`, `column_indices`, and `values` are series of equal length, one
entry per nonzero count.

## Errors

- The mapper has not been learned or loaded: the call fails with a precondition status.
- An empty document list fails with an invalid-argument status.

## Practical Examples

Three examples: the triplets of one document, the nonzero count compared with the dense matrix, and
a corpus where a document contributes nothing.

### Example 1

The triplets of a document, printed as `(row, column) = value` with the term names resolved, so the
sparse form can be read directly.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  nlp::vectorize::Mapper mapper{};
  mapper.learn(wf.infer(std::vector<std::string>{"cat sat", "dog sat"}));

  const auto matrix = mapper.count_sparse(wf.infer(std::vector<std::string>{"cat sat sat"})).value();
  const auto rows = matrix.row_indices.to(pluss::pCPU, pluss::ScalarType::Float64).contiguous().template host<double>();
  const auto cols = matrix.column_indices.to(pluss::pCPU, pluss::ScalarType::Float64).contiguous().template host<double>();
  const auto values = matrix.values.to(pluss::pCPU, pluss::ScalarType::Float64).contiguous().template host<double>();
  std::cout << "rows = " << matrix.rows << ", cols = " << matrix.cols << ", nonzeros = " << values.size() << "\n";
  for (std::size_t i = 0; i < values.size(); ++i) {
    std::cout << "  (" << rows[i] << ", " << mapper.vocabulary()[static_cast<std::size_t>(cols[i])] << ") = " << values[i] << "\n";
  }
  return 0;
}
```

Output

```text
rows = 1, cols = 3, nonzeros = 2
  (0, sat) = 2
  (0, cat) = 1
```

### Example 2

Sparse and dense agree. The program maps the same corpus both ways and compares the number of
nonzero dense entries with the number of sparse triplets, which must match.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize()}};
  const auto docs = wf.infer(std::vector<std::string>{"The cat sat. The cat ran!", "A dog sat by the door.", "Books and books."});
  nlp::vectorize::Mapper mapper{};
  mapper.learn(docs);

  const auto dense = mapper.count_dense(docs).value();
  const auto dense_values = dense.x.to(pluss::pCPU, pluss::ScalarType::Float64).contiguous().template host<double>();
  std::int64_t dense_nonzero = 0;
  for (const double value : dense_values) {
    if (value != 0.0) ++dense_nonzero;
  }
  const auto sparse = mapper.count_sparse(docs).value();
  std::cout << "dense nonzero entries = " << dense_nonzero << "\n";
  std::cout << "sparse triplets = " << sparse.values.numel() << "\n";
  std::cout << "shape = " << sparse.rows << " x " << sparse.cols << "\n";
  return 0;
}
```

Output

```text
dense nonzero entries = 12
sparse triplets = 12
shape = 3 x 10
```

### Example 3

A document with no vocabulary terms. Its row is empty in the sparse form, so the triplets name only
the documents that matched. The second document below contributes no triplets at all.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  nlp::vectorize::Mapper mapper{};
  mapper.learn(wf.infer(std::vector<std::string>{"cat sat"}));

  const auto matrix = mapper.count_sparse(wf.infer(std::vector<std::string>{"zebra", "cat"})).value();
  const auto rows = matrix.row_indices.to(pluss::pCPU, pluss::ScalarType::Float64).contiguous().template host<double>();
  std::cout << "rows = " << matrix.rows << ", triplets = " << matrix.values.numel() << "\n";
  for (const double row : rows) std::cout << "triplet in row " << row << "\n";
  return 0;
}
```

Output

```text
rows = 2, triplets = 1
triplet in row 1
```

## See Also

- [`count_dense`](count_dense.md) -- the same counts as a dense matrix.
- [`tfidf_sparse`](tfidf_sparse.md) -- the sparse form weighted by inverse document frequency.
- [`learn`](learn.md) -- builds the vocabulary this method maps over.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="count_dense.html">
    <span class="label">Previous</span>
    <span class="title">count_dense</span>
  </a>
  <a class="next-link" href="dump_state.html">
    <span class="label">Next</span>
    <span class="title">dump_state</span>
  </a>
</div>
<!-- apiref:nav:end -->
