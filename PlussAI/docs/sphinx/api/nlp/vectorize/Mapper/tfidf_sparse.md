<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 5fd159735d31 -->
# `pluss::ai::nlp::vectorize::Mapper::tfidf_sparse`

[Back to `pluss::ai::nlp::vectorize::Mapper`](../Mapper.md)

## Signature

```cpp
pluss::ai::classic::StatusOr<pluss::ai::classic::SparseTripletMatrix>
pluss::ai::nlp::vectorize::Mapper::tfidf_sparse(
    const std::vector<pluss::ai::nlp::Doc>& docs,
    const pluss::ai::classic::TfIdfMapOptions& options = {}) const;
```

## What It Does

Maps documents to TF-IDF weights as a sparse triplet matrix. It is the sparse counterpart of
[`tfidf_dense`](tfidf_dense.md): the same weighting, normalization, and column order, with only the
nonzero entries stored. The weighting formula is the one described on that page, and the inverse
document frequencies come from the corpus the mapper learned.

Use it for the same reasons as [`count_sparse`](count_sparse.md): most documents use a small part of
the vocabulary, so storing zeros wastes memory and time. A sparse weighted matrix feeds the same
classifiers as the dense one.

## Parameters

`docs` is the documents to weight, in the form the pipeline produces. Row indices run from zero to
the number of documents minus one.

`options` is the [`TfIdfMapOptions`](../../../classic/text/TfIdfMapOptions.md): `smooth_idf`,
`sublinear_tf`, and `l2_normalize`, with the same defaults as the dense method.

## Returns

A classic `StatusOr<SparseTripletMatrix>`. Its `rows` and `cols` fields give the matrix shape, and its
three series hold one entry per nonzero weight.

## Errors

- The mapper has not been learned or loaded: the call fails with a precondition status.
- An empty document list fails with an invalid-argument status.

## Practical Examples

Three examples: the sparse weights of a document, agreement with the dense weights, and the effect of
L2 normalization on the sparse values.

### Example 1

The sparse weights of one document, printed with the term names resolved.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize()}};
  const auto docs = wf.infer(std::vector<std::string>{"The cat sat.", "The dog sat.", "A cat ran!"});
  nlp::vectorize::Mapper mapper{};
  mapper.learn(docs);

  const auto matrix = mapper.tfidf_sparse(wf.infer(std::vector<std::string>{"cat cat dog"})).value();
  const auto rows = matrix.row_indices.to(pluss::pCPU, pluss::ScalarType::Float64).contiguous().template host<double>();
  const auto cols = matrix.column_indices.to(pluss::pCPU, pluss::ScalarType::Float64).contiguous().template host<double>();
  const auto values = matrix.values.to(pluss::pCPU, pluss::ScalarType::Float64).contiguous().template host<double>();
  for (std::size_t i = 0; i < values.size(); ++i) {
    std::cout << "(" << rows[i] << ", " << mapper.vocabulary()[static_cast<std::size_t>(cols[i])] << ") = " << values[i] << "\n";
  }
  return 0;
}
```

Output

```text
(0, cat) = 0.835592
(0, dog) = 0.549351
```

### Example 2

Sparse and dense weights agree. The program weights the same corpus both ways and checks that the
largest difference between any two corresponding entries is negligible.

```cpp
#include <pluss/nlp.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize()}};
  const auto docs = wf.infer(std::vector<std::string>{"The cat sat.", "The dog sat.", "A cat ran!"});
  nlp::vectorize::Mapper mapper{};
  mapper.learn(docs);

  const auto dense = mapper.tfidf_dense(docs).value();
  const auto dense_values = dense.x.to(pluss::pCPU, pluss::ScalarType::Float64).contiguous().template host<double>();
  const auto sparse = mapper.tfidf_sparse(docs).value();
  const auto rows = sparse.row_indices.to(pluss::pCPU, pluss::ScalarType::Float64).contiguous().template host<double>();
  const auto cols = sparse.column_indices.to(pluss::pCPU, pluss::ScalarType::Float64).contiguous().template host<double>();
  const auto values = sparse.values.to(pluss::pCPU, pluss::ScalarType::Float64).contiguous().template host<double>();

  double largest_difference = 0.0;
  for (std::size_t i = 0; i < values.size(); ++i) {
    const auto index = static_cast<std::size_t>(rows[i]) * static_cast<std::size_t>(dense.feature_count()) +
                       static_cast<std::size_t>(cols[i]);
    largest_difference = std::max(largest_difference, std::fabs(values[i] - dense_values[index]));
  }
  std::cout << "sparse entries = " << values.size() << "\n";
  std::cout << "largest difference from dense = " << (largest_difference < 1e-12 ? "0" : "nonzero") << "\n";
  return 0;
}
```

Output

```text
sparse entries = 9
largest difference from dense = 0
```

### Example 3

L2 normalization on the sparse values. Each document's sparse weights form a unit vector when
normalization is on, so the squared values of one row sum to one.

```cpp
#include <pluss/nlp.h>
#include <pluss/ai/ml.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize()}};
  const auto docs = wf.infer(std::vector<std::string>{"The cat sat.", "The dog sat.", "A cat ran!"});
  nlp::vectorize::Mapper mapper{};
  mapper.learn(docs);

  for (const bool normalize : {true, false}) {
    const auto matrix = mapper.tfidf_sparse(docs, pluss::ai::classic::TfIdfMapOptions{.l2_normalize = normalize}).value();
    const auto rows = matrix.row_indices.to(pluss::pCPU, pluss::ScalarType::Float64).contiguous().template host<double>();
    const auto values = matrix.values.to(pluss::pCPU, pluss::ScalarType::Float64).contiguous().template host<double>();
    double first_row_square_sum = 0.0;
    for (std::size_t i = 0; i < values.size(); ++i) {
      if (rows[i] == 0.0) first_row_square_sum += values[i] * values[i];
    }
    std::cout << "l2_normalize = " << (normalize ? "true" : "false") << ": first row squared sum = " << first_row_square_sum << "\n";
  }
  return 0;
}
```

Output

```text
l2_normalize = true: first row squared sum = 1
l2_normalize = false: first row squared sum = 4.97438
```

## See Also

- [`tfidf_dense`](tfidf_dense.md) -- the dense counterpart.
- [`count_sparse`](count_sparse.md) -- the unweighted sparse counts.
- [`TfIdfMapOptions`](../../../classic/text/TfIdfMapOptions.md) -- every weighting option.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tfidf_dense.html">
    <span class="label">Previous</span>
    <span class="title">tfidf_dense</span>
  </a>
  <a class="next-link" href="vocabulary.html">
    <span class="label">Next</span>
    <span class="title">vocabulary</span>
  </a>
</div>
<!-- apiref:nav:end -->
