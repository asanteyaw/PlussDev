# `pluss::ai::classic::text`

`text` owns two document vectorizers with no code shared between them:
[`LexiconMapper`](text/LexiconMapper.md) learns a bounded vocabulary from a
corpus, then maps documents against it as counts, presence, or TF-IDF
weights; [`HashMapper`](text/HashMapper.md) needs no corpus at all — it
hashes every n-gram directly into a fixed-width output. Like
[`online`](online.md) and [`sparse`](sparse.md), it is a small,
self-contained module: it does not participate in
[`estimators`](estimators.md)/[`compose`](compose.md)/[`tune`](tune.md)/
[`io`](io.md) the way every other family here does; see
[`Introduction`](text/Introduction.md) for exactly what that means, plus
which of the two classes to reach for.

## The Surface

| Type | What it gives you | Reach for it when |
| --- | --- | --- |
| [`LexiconMapper`](text/LexiconMapper.md) | a learned, inspectable vocabulary; counts, presence, or TF-IDF | you want a bounded, term-mapped-back-to-column-index vectorization |
| [`LexiconMapOptions`](text/LexiconMapOptions.md) | casing, vocabulary caps, document-frequency floor, n-gram range, binary counts | configuring `LexiconMapper` at construction |
| [`TfIdfMapOptions`](text/TfIdfMapOptions.md) | IDF smoothing, sublinear term frequency, L2 row normalization | configuring `LexiconMapper::map_tfidf_dense`/`map_tfidf_sparse` per call |
| [`HashMapper`](text/HashMapper.md) | a stateless hashing vectorizer, fixed output width | the vocabulary is unbounded or unknown ahead of time and a fixed memory footprint matters more than inspectability |
| [`HashMapOptions`](text/HashMapOptions.md) | casing, output width, binary counts, sign hashing, n-gram range | configuring `HashMapper` |

## How To Call It

Learn a lexicon from two sentences and map a new sentence to dense features. Hash a different document into a fixed width. The program prints each column count.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>
#include <pluss/mic.h>

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <vector>

namespace cls = pluss::ai::classic;

cls::LabeledBatch fixture_batch(const pluss::Config& cfg) {
  const pluss::Series x = pluss::make_series(std::vector<double>{
      0.0, 0.1, 0.2, 0.0, 0.1, 0.3, 0.3, 0.2,
      1.0, 1.1, 1.2, 0.9, 1.1, 1.3, 1.4, 1.0}, {8, 2}, cfg);
  const pluss::Series y = pluss::make_series(std::vector<double>{0, 0, 0, 0, 1, 1, 1, 1}, {8}, cfg);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;
  return batch;
}

int main() {
  cls::LexiconMapper mapper;
  mapper.learn({"the cat sat", "the dog ran"});
  const cls::FeatureBatch dense = mapper.map_dense({"the cat sat again"}).value();
  cls::HashMapper hasher;
  const cls::FeatureBatch hashed = hasher.map_dense({"any document at all"}).value();
  std::cout << "lexicon columns = " << dense.x.sizes()[1] << "\n";
  std::cout << "hash columns = " << hashed.x.sizes()[1] << "\n";
  return 0;
}
```

Output

```text
lexicon columns = 5
hash columns = 32768
```

## Related Pages

- [`data`](data.md) — [`FeatureBatch`](data/FeatureBatch.md), the dense
  output form both classes here can produce.
- [`sparse`](sparse.md) — [`SparseTripletMatrix`](sparse/SparseTripletMatrix.md)/
  [`SparseCsrMatrix`](sparse/SparseCsrMatrix.md), this family's other
  output form, and the learners that consume it directly.
- [`estimators`](estimators.md) — where a `FeatureBatch` produced here
  ordinarily goes next.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

text/Introduction
text/LexiconMapper
text/LexiconMapOptions
text/TfIdfMapOptions
text/HashMapper
text/HashMapOptions
```
<!-- apiref:toctree:end -->
