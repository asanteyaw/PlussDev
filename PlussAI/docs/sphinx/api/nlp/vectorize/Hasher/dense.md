<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c56e57042263 -->
# `pluss::ai::nlp::vectorize::Hasher::dense`

[Back to `pluss::ai::nlp::vectorize::Hasher`](../Hasher.md)

## Signature

```cpp
pluss::ai::classic::StatusOr<pluss::ai::classic::FeatureBatch>
pluss::ai::nlp::vectorize::Hasher::dense(const std::vector<pluss::ai::nlp::Doc>& docs) const;
pluss::ai::classic::StatusOr<pluss::ai::classic::FeatureBatch>
pluss::ai::nlp::vectorize::Hasher::dense(pluss::RangeView<std::string_view> texts) const;
```

## What It Does

Maps documents, or raw text, to a dense matrix with one row per document and `n_features` columns.
Each term adds to the column its stable hash selects. The entry is the count of terms that fell in
that column, or a presence flag with `binary` set, and a signed sum with `alternate_sign` set.

The document overload applies the hasher's [`DocMapOptions`](../DocMapOptions.md) rule, the same
feature text the mapper uses. The text overload analyzes each string directly, with no pipeline, so
it carries no lemmas and no punctuation classification. Both produce the same columns for the same
terms, so a model trained on one can score the other.

## Parameters

`docs` is the documents to map, in the form the pipeline produces.

`texts` is the corpus as text: a view of strings. Each string is analyzed as written.

## Returns

A classic `StatusOr<FeatureBatch>` with shape `{documents, n_features}`. A document with no terms
yields a zero row, not an error.

## Errors

- An n-gram range that is invalid, for example `min_ngram` greater than `max_ngram`, fails with an
  invalid-argument status.

## Practical Examples

Three examples: the hashed counts of a corpus, the same terms through the text overload, and the
signed hashing that cancels collisions.

### Example 1

Hashed counts of a small corpus with a narrow width, printed by column. Several terms share columns,
which is visible as a column with a count above one.

```cpp
#include <pluss/nlp.h>
#include <pluss/ai/ml.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  const auto docs = wf.infer(std::vector<std::string>{"cat sat dog", "bird fish"});
  nlp::vectorize::Hasher hasher{pluss::ai::classic::HashMapOptions{.n_features = 4}};

  const auto batch = hasher.dense(docs).value();
  const auto values = batch.x.to(pluss::pCPU, pluss::ScalarType::Float64).contiguous().template host<double>();
  for (std::int64_t row = 0; row < batch.sample_count(); ++row) {
    std::cout << "document " << row << ":";
    for (std::int64_t column = 0; column < batch.feature_count(); ++column) {
      std::cout << " " << values[static_cast<std::size_t>(row * batch.feature_count() + column)];
    }
    std::cout << "\n";
  }
  return 0;
}
```

Output

```text
document 0: 0 2 0 1
document 1: 1 1 0 0
```

### Example 2

The text overload. Raw strings are mapped without a pipeline, and the row matches the one the document
route produces for the same words, because both use the same column rule.

```cpp
#include <pluss/nlp.h>
#include <pluss/ai/ml.h>

#include <iostream>
#include <string>
#include <string_view>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  const nlp::vectorize::Hasher hasher{pluss::ai::classic::HashMapOptions{.n_features = 8}};
  const std::vector<std::string_view> texts = {"cat sat", "dog"};
  const auto batch = hasher.dense(texts).value();
  const auto values = batch.x.to(pluss::pCPU, pluss::ScalarType::Float64).contiguous().template host<double>();
  std::cout << "shape = " << batch.sample_count() << " x " << batch.feature_count() << "\n";
  std::cout << "row 0 sum = ";
  double sum = 0.0;
  for (std::int64_t column = 0; column < batch.feature_count(); ++column) sum += values[static_cast<std::size_t>(column)];
  std::cout << sum << "\n";
  return 0;
}
```

Output

```text
shape = 2 x 8
row 0 sum = 2
```

### Example 3

Signed hashing. With `alternate_sign` on, each term adds with a sign taken from its hash, so the
column totals can differ from the unsigned totals even though the same terms are present.

```cpp
#include <pluss/nlp.h>
#include <pluss/ai/ml.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  const auto docs = wf.infer(std::vector<std::string>{"cat sat dog bird zebra"});
  for (const bool signed_hash : {false, true}) {
    const nlp::vectorize::Hasher hasher{pluss::ai::classic::HashMapOptions{.n_features = 4, .alternate_sign = signed_hash}};
    const auto batch = hasher.dense(docs).value();
    const auto values = batch.x.to(pluss::pCPU, pluss::ScalarType::Float64).contiguous().template host<double>();
    std::cout << "alternate_sign = " << (signed_hash ? "true" : "false") << ":";
    for (const double value : values) std::cout << " " << value;
    std::cout << "\n";
  }
  return 0;
}
```

Output

```text
alternate_sign = false: 1 3 0 1
alternate_sign = true: 1 1 0 1
```

## See Also

- [`sparse`](sparse.md) -- the same mapping as a sparse matrix.
- [`Hasher`](../Hasher.md) -- the vectorizer and its options.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="sparse.html">
    <span class="label">Next</span>
    <span class="title">sparse</span>
  </a>
</div>
<!-- apiref:nav:end -->
