<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c4e3d66e1014 -->
# `pluss::ai::nlp::vectorize::Mapper::count_dense`

[Back to `pluss::ai::nlp::vectorize::Mapper`](../Mapper.md)

## Signature

```cpp
pluss::ai::classic::StatusOr<pluss::ai::classic::FeatureBatch>
pluss::ai::nlp::vectorize::Mapper::count_dense(const std::vector<pluss::ai::nlp::Doc>& docs) const;
```

## What It Does

Maps documents to a dense matrix over the learned vocabulary. Each row is one document and each
column is one vocabulary term, in vocabulary order. The entry is the number of times the term
occurs in that document, or a presence flag when the mapper was built with `binary` set. Terms the
document does not contain are zero, and terms the document contains that are not in the vocabulary
are dropped: they are not counted anywhere, so an unseen word at inference time costs the row
nothing.

The method runs the same feature-text rule as [`learn`](learn.md), under the mapper's
`DocMapOptions`, so a document is counted the same way at training and at inference. It does not
change the mapper.

## Parameters

`docs` is the documents to map, in the form the pipeline produces. Each document becomes one row,
so the result has as many rows as there are documents, including empty ones.

## Returns

A classic `StatusOr<FeatureBatch>`. On success the batch holds a `Series` with shape
`{documents, vocabulary size}`, read through `sample_count()` and `feature_count()`. Read the values
with `x.to(...)` and `to_vec`.

## Errors

- The mapper has not been learned or loaded: the call fails with a precondition status. Check
  `vocabulary().empty()` first if the mapper may be untrained.
- An empty document list fails with an invalid-argument status.

## Practical Examples

Three examples: the dense count matrix of a small corpus, the effect of a term the vocabulary does
not hold, and the failure before learning.

### Example 1

Mapping a corpus to counts. The output prints the matrix with each term's column name, so every
count can be checked against the document.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize()}};
  const auto docs = wf.infer(std::vector<std::string>{"The cat sat. The cat ran!", "A dog sat by the door."});
  nlp::vectorize::Mapper mapper{};
  mapper.learn(docs);

  const auto batch = mapper.count_dense(docs).value();
  const auto values = batch.x.to(pluss::pCPU, pluss::ScalarType::Float64).contiguous().template to_vec<double>();
  const std::int64_t cols = batch.feature_count();
  for (std::int64_t row = 0; row < batch.sample_count(); ++row) {
    std::cout << "document " << row << ":";
    for (std::int64_t col = 0; col < cols; ++col) {
      std::cout << " " << mapper.vocabulary()[static_cast<std::size_t>(col)] << "=" << values[static_cast<std::size_t>(row * cols + col)];
    }
    std::cout << "\n";
  }
  return 0;
}
```

Output

```text
document 0: sat=1 the=2 a=0 by=0 cat=2 dog=0 door=0 ran=1
document 1: sat=1 the=1 a=1 by=1 cat=0 dog=1 door=1 ran=0
```

### Example 2

A term the vocabulary does not hold. The mapper learned on one corpus, and the inference document
contains an unseen word and a known one. The unseen word is dropped and the known one is counted.

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

  const auto unseen = wf.infer(std::vector<std::string>{"zebra cat"});
  const auto batch = mapper.count_dense(unseen).value();
  const auto values = batch.x.to(pluss::pCPU, pluss::ScalarType::Float64).contiguous().template to_vec<double>();
  std::cout << "vocabulary:";
  for (const std::string& term : mapper.vocabulary()) std::cout << " " << term;
  std::cout << "\n";
  std::cout << "row shape = " << batch.sample_count() << " x " << batch.feature_count() << "\n";
  std::cout << "counts:";
  for (const double value : values) std::cout << " " << value;
  std::cout << "\n";
  return 0;
}
```

Output

```text
vocabulary: sat cat dog
row shape = 1 x 3
counts: 0 1 0
```

### Example 3

The failure before learning. A fresh mapper has no vocabulary, so mapping fails with a status
that says so, and the program prints the message's first line.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  const nlp::vectorize::Mapper mapper{};
  const auto result = mapper.count_dense(wf.infer(std::vector<std::string>{"cat sat"}));
  std::cout << "ok = " << result.ok() << "\n";
  const std::string message = result.status().message;
  std::cout << message.substr(0, message.find('\n')) << "\n";
  return 0;
}
```

Output

```text
ok = 0
LexiconMapper::map_dense requires learn() first.
```

## See Also

- [`count_sparse`](count_sparse.md) -- the same counts as a sparse matrix.
- [`tfidf_dense`](tfidf_dense.md) -- the same rows, weighted by inverse document frequency.
- [`learn`](learn.md) -- builds the vocabulary this method maps over.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="count_sparse.html">
    <span class="label">Next</span>
    <span class="title">count_sparse</span>
  </a>
</div>
<!-- apiref:nav:end -->
