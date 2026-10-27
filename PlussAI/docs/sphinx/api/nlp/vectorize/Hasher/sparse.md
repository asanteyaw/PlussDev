<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 7e0ac58d6d70 -->
# `pluss::ai::nlp::vectorize::Hasher::sparse`

[Back to `pluss::ai::nlp::vectorize::Hasher`](../Hasher.md)

## Signature

```cpp
pluss::ai::classic::StatusOr<pluss::ai::classic::SparseTripletMatrix>
pluss::ai::nlp::vectorize::Hasher::sparse(const std::vector<pluss::ai::nlp::Doc>& docs) const;
pluss::ai::classic::StatusOr<pluss::ai::classic::SparseTripletMatrix>
pluss::ai::nlp::vectorize::Hasher::sparse(pluss::RangeView<std::string_view> texts) const;
```

## What It Does

Maps documents, or raw text, to the same hashed counts as [`dense`](dense.md), stored as a triplet
matrix that keeps only the nonzero entries. Each triplet names a document row, a hashed column, and
the count or signed sum in that cell.

Hashing spreads a document's terms over `n_features` columns, so a document touches few of them, and
sparse storage is the natural form for large widths. The two overloads follow the same rules as the
dense methods: the document form applies the hasher's `DocMapOptions`, and the text form analyzes the
strings as written.

## Parameters

`docs` is the documents to map, in the form the pipeline produces.

`texts` is the corpus as text: a view of strings, each analyzed as written.

## Returns

A classic `StatusOr<SparseTripletMatrix>`. Its `rows` field is the number of documents, `cols` is
`n_features`, and the three series hold one entry per nonzero cell.

## Errors

- An n-gram range that is invalid fails with an invalid-argument status.

## Practical Examples

Three examples: the triplets of a document, the sparse and dense forms of the same mapping, and a wide
hasher whose sparse matrix holds few entries.

### Example 1

The triplets of a document under a narrow width, printed with the hashed column numbers.

```cpp
#include <pluss/nlp.h>
#include <pluss/ai/ml.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  const nlp::vectorize::Hasher hasher{pluss::ai::classic::HashMapOptions{.n_features = 4}};
  const auto matrix = hasher.sparse(wf.infer(std::vector<std::string>{"cat sat cat"})).value();
  const auto rows = matrix.row_indices.to(pluss::pCPU, pluss::ScalarType::Float64).contiguous().template host<double>();
  const auto cols = matrix.column_indices.to(pluss::pCPU, pluss::ScalarType::Float64).contiguous().template host<double>();
  const auto values = matrix.values.to(pluss::pCPU, pluss::ScalarType::Float64).contiguous().template host<double>();
  std::cout << "rows = " << matrix.rows << ", cols = " << matrix.cols << ", nonzeros = " << values.size() << "\n";
  for (std::size_t i = 0; i < values.size(); ++i) {
    std::cout << "  (" << rows[i] << ", " << cols[i] << ") = " << values[i] << "\n";
  }
  return 0;
}
```

Output

```text
```

### Example 2

The sparse and dense forms of one mapping. The program maps the same documents both ways and compares
the nonzero counts, which must agree.

```cpp
#include <pluss/nlp.h>
#include <pluss/ai/ml.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  const auto docs = wf.infer(std::vector<std::string>{"cat sat dog", "bird fish", "cat cat"});
  const nlp::vectorize::Hasher hasher{pluss::ai::classic::HashMapOptions{.n_features = 16}};

  const auto dense = hasher.dense(docs).value();
  const auto dense_values = dense.x.to(pluss::pCPU, pluss::ScalarType::Float64).contiguous().template host<double>();
  std::int64_t dense_nonzero = 0;
  for (const double value : dense_values) {
    if (value != 0.0) ++dense_nonzero;
  }
  const auto sparse = hasher.sparse(docs).value();
  std::cout << "dense nonzero = " << dense_nonzero << ", sparse triplets = " << sparse.values.numel() << "\n";
  return 0;
}
```

Output

```text
```

### Example 3

A wide hasher. With `n_features` set to 2^15, a short document touches only a handful of the columns,
so the sparse matrix holds a few entries against the tens of thousands of columns.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  const nlp::vectorize::Hasher hasher{};
  const auto matrix = hasher.sparse(wf.infer(std::vector<std::string>{"cat sat dog"})).value();
  std::cout << "shape = " << matrix.rows << " x " << matrix.cols << "\n";
  std::cout << "nonzero triplets = " << matrix.values.numel() << "\n";
  return 0;
}
```

Output

```text
shape = 1 x 32768
nonzero triplets = 3
```

## See Also

- [`dense`](dense.md) -- the same mapping as a dense matrix.
- [`Hasher`](../Hasher.md) -- the vectorizer and its options.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dense.html">
    <span class="label">Previous</span>
    <span class="title">dense</span>
  </a>
</div>
<!-- apiref:nav:end -->
