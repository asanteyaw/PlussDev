<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e18bccd930a2 -->
# `pluss::ai::nlp::vectorize::Mapper::tfidf_dense`

[Back to `pluss::ai::nlp::vectorize::Mapper`](../Mapper.md)

## Signature

```cpp
pluss::ai::classic::StatusOr<pluss::ai::classic::FeatureBatch>
pluss::ai::nlp::vectorize::Mapper::tfidf_dense(
    const std::vector<pluss::ai::nlp::Doc>& docs,
    const pluss::ai::classic::TfIdfMapOptions& options = {}) const;
```

## What It Does

Maps documents to TF-IDF weights over the learned vocabulary. The counts from
[`count_dense`](count_dense.md) are weighted by each term's inverse document frequency, so a term
that appears in most documents contributes less than one that is rare. Each row is then normalized,
so a long document and a short one are comparable.

The weighting, for a term with document frequency `df` in a corpus of `n` documents, is:

- the inverse document frequency is `ln((1 + n) / (1 + df)) + 1` with `smooth_idf` on, and
  `ln(n / df) + 1` with it off;
- the term frequency is the raw count, or `1 + ln(count)` with `sublinear_tf` on;
- the weight is the term frequency times the inverse document frequency, and with `l2_normalize` on
  each row is divided by its Euclidean length.

The inverse document frequencies come from the corpus the mapper learned, not the documents passed
here, so a single-document call uses the training statistics.

## Parameters

`docs` is the documents to weight, in the form the pipeline produces.

`options` is the [`TfIdfMapOptions`](../../../classic/text/TfIdfMapOptions.md): `smooth_idf` (default
on), `sublinear_tf` (default off), and `l2_normalize` (default on).

## Returns

A classic `StatusOr<FeatureBatch>` with shape `{documents, vocabulary size}`, read like the batch
from [`count_dense`](count_dense.md).

## Errors

- The mapper has not been learned or loaded: the call fails with a precondition status.
- An empty document list fails with an invalid-argument status.

## Practical Examples

Three examples: the TF-IDF weights of a corpus, the effect of the smoothing and normalization options,
and the sublinear term frequency on a repetitive document.

### Example 1

TF-IDF weights of a corpus, with a common term and a rare one shown side by side, so the
down-weighting of the common term is visible.

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

  const auto batch = mapper.tfidf_dense(docs).value();
  const auto values = batch.x.to(pluss::pCPU, pluss::ScalarType::Float64).contiguous().template to_vec<double>();
  const std::int64_t cols = batch.feature_count();
  for (std::int64_t row = 0; row < batch.sample_count(); ++row) {
    std::cout << "document " << row << ":";
    for (std::int64_t col = 0; col < cols; ++col) {
      std::cout << " " << mapper.vocabulary()[static_cast<std::size_t>(col)] << "="
                << values[static_cast<std::size_t>(row * cols + col)];
    }
    std::cout << "\n";
  }
  return 0;
}
```

Output

```text
document 0: cat=0.57735 sat=0.57735 the=0.57735 a=0 dog=0 ran=0
document 1: cat=0 sat=0.517856 the=0.517856 a=0 dog=0.680919 ran=0
document 2: cat=0.47363 sat=0 the=0 a=0.622766 dog=0 ran=0.622766
```

### Example 2

The smoothing and normalization options. The same document is weighted under three settings, so the
effect of `smooth_idf` and `l2_normalize` on the same row can be compared.

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

  const std::pair<const char*, pluss::ai::classic::TfIdfMapOptions> settings[] = {
      {"default", {}},
      {"smooth_idf = false", {.smooth_idf = false}},
      {"l2_normalize = false", {.l2_normalize = false}},
  };
  for (const auto& [name, options] : settings) {
    const auto batch = mapper.tfidf_dense(docs, options).value();
    const auto values = batch.x.to(pluss::pCPU, pluss::ScalarType::Float64).contiguous().template to_vec<double>();
    std::cout << name << ": first document =";
    for (std::int64_t col = 0; col < batch.feature_count(); ++col) {
      std::cout << " " << values[static_cast<std::size_t>(col)];
    }
    std::cout << "\n";
  }
  return 0;
}
```

Output

```text
default: first document = 0.57735 0.57735 0.57735 0 0 0
smooth_idf = false: first document = 0.57735 0.57735 0.57735 0 0 0
l2_normalize = false: first document = 1.28768 1.28768 1.28768 0 0 0
```

### Example 3

Sublinear term frequency. A document that repeats a word is weighted by `1 + ln(count)` rather than the
raw count, so a term repeated many times does not dominate the row.

```cpp
#include <pluss/nlp.h>
#include <pluss/ai/ml.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize()}};
  nlp::vectorize::Mapper mapper{};
  mapper.learn(wf.infer(std::vector<std::string>{"cat sat", "dog sat"}));

  const auto repeated = wf.infer(std::vector<std::string>{"cat cat cat sat"});
  for (const bool sublinear : {false, true}) {
    const auto batch = mapper.tfidf_dense(repeated, pluss::ai::classic::TfIdfMapOptions{.sublinear_tf = sublinear,
                                                                                      .l2_normalize = false}).value();
    const auto values = batch.x.to(pluss::pCPU, pluss::ScalarType::Float64).contiguous().template to_vec<double>();
    std::cout << "sublinear_tf = " << (sublinear ? "true" : "false") << ": cat = " << values[0]
              << ", sat = " << values[1] << "\n";
  }
  return 0;
}
```

Output

```text
sublinear_tf = false: cat = 1, sat = 4.2164
sublinear_tf = true: cat = 1, sat = 2.94953
```

## See Also

- [`tfidf_sparse`](tfidf_sparse.md) -- the same weights as a sparse matrix.
- [`count_dense`](count_dense.md) -- the unweighted counts.
- [`TfIdfMapOptions`](../../../classic/text/TfIdfMapOptions.md) -- every weighting option.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="load_state.html">
    <span class="label">Previous</span>
    <span class="title">load_state</span>
  </a>
  <a class="next-link" href="tfidf_sparse.html">
    <span class="label">Next</span>
    <span class="title">tfidf_sparse</span>
  </a>
</div>
<!-- apiref:nav:end -->
