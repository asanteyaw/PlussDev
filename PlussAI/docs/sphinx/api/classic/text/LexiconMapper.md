# `pluss::ai::classic::LexiconMapper`

Learns a bounded vocabulary from a document corpus, then vectorizes new
documents against it as raw counts, presence/absence, or TF-IDF weights —
dense or sparse. The vocabulary-backed counterpart of
[`HashMapper`](HashMapper.md): every output column corresponds to a real,
inspectable term (`vocabulary()`), at the cost of a `learn()` step and a
vocabulary that grows with the corpus, rather than a fixed hashed width.

## Signature

```cpp
explicit classic::LexiconMapper::LexiconMapper(classic::LexiconMapOptions options = {});

bool classic::LexiconMapper::learned() const noexcept;
std::int64_t classic::LexiconMapper::feature_count() const noexcept;

classic::Status classic::LexiconMapper::learn(
    /* a range of std::string -- pass a std::vector<std::string> or braced list */ documents);

classic::StatusOr<classic::FeatureBatch> classic::LexiconMapper::map_dense(
    /* a range of std::string */ documents) const;
classic::StatusOr<classic::SparseTripletMatrix> classic::LexiconMapper::map_sparse(
    /* a range of std::string */ documents) const;
classic::StatusOr<classic::FeatureBatch> classic::LexiconMapper::map_tfidf_dense(
    /* a range of std::string */ documents,
    const classic::TfIdfMapOptions& options = {}) const;
classic::StatusOr<classic::SparseTripletMatrix> classic::LexiconMapper::map_tfidf_sparse(
    /* a range of std::string */ documents,
    const classic::TfIdfMapOptions& options = {}) const;

const std::vector<std::string>& classic::LexiconMapper::vocabulary() const noexcept;

classic::LexiconMapperState classic::LexiconMapper::dump_state() const;
classic::Status classic::LexiconMapper::load_state(const classic::LexiconMapperState& state);
```

`learn`, `map_dense`, `map_sparse`, `map_tfidf_dense`, and
`map_tfidf_sparse` each also have a second overload taking
`std::initializer_list<std::string>` in place of the range — the common
case when documents are spelled directly as a braced `{...}` list. Both
forms are exercised throughout the examples below.

## Construction

`LexiconMapper(options = {})` takes a
[`LexiconMapOptions`](LexiconMapOptions.md) and never fails — every field
is validated lazily, the first time it actually matters
([`learn`](#methods) or [`load_state`](#methods)), not at construction. A
`LexiconMapper` built with, say, `max_features = 0` constructs cleanly and
only fails once `learn()` is actually called.

## State

`learned()` — `bool`
: `true` once [`learn`](#methods) (or a successful
  [`load_state`](#methods)) has established a vocabulary. Every `map_*`
  method requires this.

`feature_count()` — `std::int64_t`
: The current vocabulary size — `0` before `learn`, otherwise
  `vocabulary().size()`. Every `map_*` output's column count equals this.

`vocabulary()` — `const std::vector<std::string>&`
: The learned terms, in the exact order they occupy output columns —
  index `i` here is column `i` of every `map_dense`/`map_tfidf_dense`
  output.

## Methods

`learn(documents)`
: Builds the vocabulary from scratch, discarding any previous one.
  Tokenizes each document (ASCII-lowercased when
  [`LexiconMapOptions::lowercase`](LexiconMapOptions.md#lowercase) is set,
  split into n-grams per
  [`min_ngram`/`max_ngram`](LexiconMapOptions.md#min_ngram)),
  deduplicates each document's own terms before counting document
  frequency (so a term repeating within one document counts once toward
  that floor), drops any term below
  [`min_document_frequency`](LexiconMapOptions.md#min_document_frequency),
  sorts the survivors by document frequency descending (ties broken
  alphabetically ascending), and keeps the top
  [`max_features`](LexiconMapOptions.md#max_features). `documents` must be
  non-empty; every `LexiconMapOptions` field is validated first (see
  `## Errors`); if literally no term survives the frequency floor, `learn`
  fails rather than producing an empty vocabulary.

`map_dense(documents)`
: `[rows, feature_count()]` — each cell is the count of that column's term
  in that row's document (or `1.0`/`0.0` if
  [`binary`](LexiconMapOptions.md#binary) is set), using the **same**
  tokenization options `learn` used. A term in `documents` that is not in
  the learned vocabulary (out-of-vocabulary) contributes nothing — never
  an error, silently ignored. Requires [`learned()`](#state).

`map_sparse(documents)`
: The same values as `map_dense`, as a
  [`SparseTripletMatrix`](../sparse/SparseTripletMatrix.md) instead —
  proven numerically identical in [Example 2](#example-2). Unlike
  converting that result through
  [`sparse_triplet_to_csr`](../sparse/make_sparse_triplet.md), this method
  never produces a duplicate `(row, column)` pair to begin with — each
  row's nonzero columns are accumulated in a hash map first, so every
  `(row, column)` pair here is already unique.

`map_tfidf_dense(documents, options = {})` / `map_tfidf_sparse(documents, options = {})`
: The same raw counts as `map_dense`/`map_sparse`, reweighted by
  [`TfIdfMapOptions`](TfIdfMapOptions.md) — see that page for the exact
  IDF formulas and the effect of each field. **The IDF weights are fixed
  at `learn` time** (document frequency and corpus size, both captured
  when `learn` ran) — calling `map_tfidf_*` on a *different* set of
  documents than was learned reweights those new documents by the
  original training corpus's statistics, the standard fit-once/
  transform-many contract. See [Example 8](#example-8).

`dump_state()` / `load_state(state)`
: `LexiconMapperState` — a plain struct, not a `Series` vector (unlike
  every state-dump type in [`sparse`](../sparse.md)):

  ```cpp
  struct classic::LexiconMapperState {
    classic::LexiconMapOptions options;
    bool learned;
    std::int64_t learned_document_count;
    std::vector<std::string> vocabulary;
    std::vector<std::int64_t> document_frequency;
  };
  ```

  `load_state` re-validates `state.options` exactly like `learn` does
  (`max_features > 0`, `min_document_frequency > 0`, a valid n-gram
  range), and additionally requires `vocabulary.size() ==
  document_frequency.size()` and, if `learned` is `true`, a non-empty
  `vocabulary`. See [Example 12](#example-12) for a round trip.

## Errors

- `learn`: `documents` empty — `Status::invalid_argument`.
- `learn`: `options.max_features <= 0`, `options.min_document_frequency
  <= 0`, or an invalid n-gram range (either `<= 0`, or `min_ngram >
  max_ngram`) — `Status::invalid_argument`.
- `learn`: no term survives `min_document_frequency` —
  `Status::invalid_argument`.
- `map_dense` / `map_sparse` / `map_tfidf_dense` / `map_tfidf_sparse`:
  called before a successful `learn`/`load_state` — `Status::precondition`.
- `map_dense` / `map_sparse` / `map_tfidf_dense` / `map_tfidf_sparse`:
  `documents` empty — `Status::invalid_argument`.
- `load_state`: `state.options.max_features <= 0`,
  `min_document_frequency <= 0`, or an invalid n-gram range —
  `Status::invalid_argument`.
- `load_state`: `vocabulary`/`document_frequency` length mismatch —
  `Status::shape_mismatch`.
- `load_state`: `learned = true` with an empty `vocabulary` —
  `Status::invalid_argument`.

## Practical Examples

The full `learn`/`map_dense`/`map_sparse` lifecycle, the two forms proven
numerically identical, `min_document_frequency` filtering rare terms,
`max_features`'s exact ranking and cutoff, `binary` collapsing counts,
bigrams via `min_ngram`/`max_ngram`, a document too short for the
requested n-gram range, out-of-vocabulary terms silently ignored, TF-IDF
scored against a held-out document, `smooth_idf`'s formula difference,
`sublinear_tf`'s dampening, `l2_normalize` turned off, a `dump_state`/
`load_state` round trip, and an error roundup for both `learn` and
`load_state`.

### Example 1

Four short documents, default options — lowercase, unigrams only, no
document-frequency floor beyond the default `1`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LexiconMapper mapper;
  const cls::Status learned = mapper.learn(
      {"the cat sat", "the dog sat", "cats and dogs", "the cat ran"});
  std::cout << "learn ok = " << std::boolalpha << learned.ok() << "\n";
  std::cout << "learned() = " << mapper.learned() << "\n";
  std::cout << "feature_count() = " << mapper.feature_count() << "\n";

  const cls::FeatureBatch batch = mapper.map_dense({"the cat sat again"}).value();
  std::cout << "map_dense = " << batch.x << "\n";

  return 0;
}
```

Output

```text
learn ok = true
learned() = true
feature_count() = 8
map_dense = pluss::Series(shape=[1,8], strides=[8,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[1, 1, ... , 0]]
```

### Example 2

`map_sparse` proven numerically identical to `map_dense`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LexiconMapper mapper;
  mapper.learn({"the cat sat", "the dog sat", "cats and dogs", "the cat ran"});

  const cls::FeatureBatch dense = mapper.map_dense({"the cat sat again", "dogs ran"}).value();
  const cls::SparseTripletMatrix sparse = mapper.map_sparse({"the cat sat again", "dogs ran"}).value();
  const cls::SparseCsrMatrix csr = cls::sparse_triplet_to_csr(sparse).value();

  std::vector<double> dense_from_sparse(static_cast<std::size_t>(csr.rows * csr.cols), 0.0);
  for (std::int64_t r = 0; r < csr.rows; ++r) {
    for (std::int64_t p = csr.row_ptr[static_cast<std::size_t>(r)]; p < csr.row_ptr[static_cast<std::size_t>(r + 1)]; ++p) {
      dense_from_sparse[static_cast<std::size_t>(r * csr.cols + csr.col_ind[static_cast<std::size_t>(p)])] = csr.values[static_cast<std::size_t>(p)];
    }
  }
  const pluss::Series reconstructed = pluss::make_series(
      dense_from_sparse, {csr.rows, csr.cols}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));
  const double diff = (dense.x - reconstructed).abs().amax(0, false).amax(0, false).item<double>();
  std::cout << "max abs diff (dense vs. reconstructed-from-sparse) = " << diff << "\n";

  return 0;
}
```

Output

```text
max abs diff (dense vs. reconstructed-from-sparse) = 0
```

### Example 3

`min_document_frequency = 2` drops any term seen in only one document.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LexiconMapOptions options{};
  options.min_document_frequency = 2;
  cls::LexiconMapper mapper(options);

  // "unique" appears in exactly one document; "the"/"cat" appear in two+.
  mapper.learn({"the cat sat", "the cat ran", "a unique word appears once"});
  std::cout << "feature_count() = " << mapper.feature_count() << "\n";
  std::cout << "vocabulary = [";
  for (const auto& term : mapper.vocabulary()) std::cout << "\"" << term << "\" ";
  std::cout << "]\n";

  return 0;
}
```

Output

```text
feature_count() = 2
vocabulary = ["cat" "the" ]
```

### Example 4

`max_features = 2` keeps only the two most-frequent terms, ties broken
alphabetically.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LexiconMapOptions options{};
  options.max_features = 2;
  cls::LexiconMapper mapper(options);

  // "the" appears in 3 docs; "cat" and "dog" each appear in 2 (a tie,
  // broken alphabetically: "cat" before "dog"); "ran"/"sat" each in 1.
  mapper.learn({"the cat sat", "the cat ran", "the dog sat", "a dog ran too"});
  std::cout << "vocabulary (kept, in order) = [";
  for (const auto& term : mapper.vocabulary()) std::cout << "\"" << term << "\" ";
  std::cout << "]\n";

  return 0;
}
```

Output

```text
vocabulary (kept, in order) = ["the" "cat" ]
```

### Example 5

`binary = true` collapses repeated occurrences to a flat `1.0`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LexiconMapper counts;
  counts.learn({"cat cat cat dog"});

  cls::LexiconMapOptions binary_options{};
  binary_options.binary = true;
  cls::LexiconMapper presence(binary_options);
  presence.learn({"cat cat cat dog"});

  std::cout << "counts    = " << counts.map_dense({"cat cat cat dog"}).value().x << "\n";
  std::cout << "presence  = " << presence.map_dense({"cat cat cat dog"}).value().x << "\n";

  return 0;
}
```

Output

```text
counts    = pluss::Series(shape=[1,2], strides=[2,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[3, 1]]
presence  = pluss::Series(shape=[1,2], strides=[2,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[1, 1]]
```

### Example 6

`min_ngram = max_ngram = 2` (bigrams only) — single tokens are not kept as
features at all.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LexiconMapOptions options{};
  options.min_ngram = 2;
  options.max_ngram = 2;
  cls::LexiconMapper mapper(options);

  mapper.learn({"the cat sat", "the cat ran"});
  std::cout << "vocabulary (bigrams) = [";
  for (const auto& term : mapper.vocabulary()) std::cout << "\"" << term << "\" ";
  std::cout << "]\n";

  return 0;
}
```

Output

```text
vocabulary (bigrams) = ["the cat" "cat ran" "cat sat" ]
```

### Example 7

A document shorter than `min_ngram` contributes zero grams — silently, not
an error — while a longer document in the same batch still contributes
normally.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LexiconMapOptions options{};
  options.min_ngram = 3;
  options.max_ngram = 3;
  cls::LexiconMapper mapper(options);

  // "ok" (1 token) and "a b" (2 tokens) are both too short for a trigram;
  // "the cat sat down" (4 tokens) contributes two trigrams.
  const cls::Status learned = mapper.learn({"the cat sat down", "ok", "a b"});
  std::cout << "learn ok = " << std::boolalpha << learned.ok() << "\n";
  std::cout << "feature_count() = " << mapper.feature_count() << "\n";

  return 0;
}
```

Output

```text
learn ok = true
feature_count() = 2
```

### Example 8

A term never seen during `learn` is silently ignored at `map_dense` time —
it contributes nothing, the row is simply shorter on features, not an
error.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LexiconMapper mapper;
  mapper.learn({"the cat sat"});

  const cls::FeatureBatch batch = mapper.map_dense({"the cat flew to jupiter"}).value();
  std::cout << "feature_count() = " << mapper.feature_count() << "\n";
  std::cout << "map_dense (unseen terms ignored) = " << batch.x << "\n";

  return 0;
}
```

Output

```text
feature_count() = 3
map_dense (unseen terms ignored) = pluss::Series(shape=[1,3], strides=[3,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[[1, 0, 1]]
```

### Example 9

`smooth_idf` computed both ways and compared by hand against the real
output — `learn` on 4 documents, a term with document frequency `2`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LexiconMapper mapper;
  mapper.learn({"cat sat", "cat ran", "dog sat", "dog ran"});
  // "cat" has document_frequency = 2, corpus n_docs = 4.

  cls::TfIdfMapOptions smoothed{};
  smoothed.l2_normalize = false;
  cls::TfIdfMapOptions unsmoothed{};
  unsmoothed.smooth_idf = false;
  unsmoothed.l2_normalize = false;

  std::cout << "smoothed   (expect idf(cat) = ln(5/3)+1 = 1.5108256) = "
            << mapper.map_tfidf_dense({"cat"}, smoothed).value().x << "\n";
  std::cout << "unsmoothed (expect idf(cat) = ln(4/2)+1 = 1.6931472) = "
            << mapper.map_tfidf_dense({"cat"}, unsmoothed).value().x << "\n";

  return 0;
}
```

Output

```text
smoothed   (expect idf(cat) = ln(5/3)+1 = 1.5108256) = pluss::Series(shape=[1,4], strides=[4,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1.5108256, 0, 0, 0]]
unsmoothed (expect idf(cat) = ln(4/2)+1 = 1.6931472) = pluss::Series(shape=[1,4], strides=[4,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1.6931472, 0, 0, 0]]
```

### Example 10

`sublinear_tf` dampens a term repeated many times in one document.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LexiconMapper mapper;
  mapper.learn({"cat sat", "dog ran"});

  cls::TfIdfMapOptions linear{};
  linear.l2_normalize = false;
  cls::TfIdfMapOptions sublinear{};
  sublinear.sublinear_tf = true;
  sublinear.l2_normalize = false;

  // "cat" repeats 5 times: linear tf = 5, sublinear tf = 1 + ln(5) = 2.6094379.
  std::cout << "linear tf    = " << mapper.map_tfidf_dense({"cat cat cat cat cat"}, linear).value().x << "\n";
  std::cout << "sublinear tf = " << mapper.map_tfidf_dense({"cat cat cat cat cat"}, sublinear).value().x << "\n";

  return 0;
}
```

Output

```text
linear tf    = pluss::Series(shape=[1,4], strides=[4,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[7.0273255, 0, 0, 0]]
sublinear tf = pluss::Series(shape=[1,4], strides=[4,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[3.6674739, 0, 0, 0]]
```

### Example 11

`l2_normalize = false` leaves rows at their raw IDF-weighted magnitude
instead of unit length.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LexiconMapper mapper;
  mapper.learn({"cat sat", "dog ran", "cat ran"});

  cls::TfIdfMapOptions normalized{};
  cls::TfIdfMapOptions raw{};
  raw.l2_normalize = false;

  const pluss::Series normalized_row = mapper.map_tfidf_dense({"cat ran fast"}, normalized).value().x;
  const pluss::Series raw_row = mapper.map_tfidf_dense({"cat ran fast"}, raw).value().x;
  std::cout << "normalized row L2 norm = "
            << normalized_row.square().sum(1, false).sqrt().item<double>() << "\n";
  std::cout << "raw row L2 norm        = "
            << raw_row.square().sum(1, false).sqrt().item<double>() << "\n";

  return 0;
}
```

Output

```text
normalized row L2 norm = 1
raw row L2 norm        = 1.82106
```

### Example 12

TF-IDF scored against a document *not* in the training corpus still uses
the training corpus's own IDF statistics — a genuinely new document never
changes `document_frequency`/`learned_document_count`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LexiconMapper mapper;
  mapper.learn({"cat sat", "cat ran", "dog sat", "dog ran"});

  cls::TfIdfMapOptions options{};
  options.l2_normalize = false;
  const pluss::Series held_out = mapper.map_tfidf_dense({"cat appeared in a brand new document"}, options).value().x;
  std::cout << "held-out document's TF-IDF row (cat's idf still from training) = " << held_out << "\n";

  return 0;
}
```

Output

```text
held-out document's TF-IDF row (cat's idf still from training) = pluss::Series(shape=[1,4], strides=[4,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1.5108256, 0, 0, 0]]
```

### Example 13

`map_tfidf_sparse` proven numerically identical to `map_tfidf_dense`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LexiconMapper mapper;
  mapper.learn({"cat sat", "cat ran", "dog sat", "dog ran"});

  const pluss::Series dense = mapper.map_tfidf_dense({"cat ran home", "dog sat still"}).value().x;
  const cls::SparseTripletMatrix sparse = mapper.map_tfidf_sparse({"cat ran home", "dog sat still"}).value();
  const cls::SparseCsrMatrix csr = cls::sparse_triplet_to_csr(sparse).value();

  std::vector<double> dense_from_sparse(static_cast<std::size_t>(csr.rows * csr.cols), 0.0);
  for (std::int64_t r = 0; r < csr.rows; ++r) {
    for (std::int64_t p = csr.row_ptr[static_cast<std::size_t>(r)]; p < csr.row_ptr[static_cast<std::size_t>(r + 1)]; ++p) {
      dense_from_sparse[static_cast<std::size_t>(r * csr.cols + csr.col_ind[static_cast<std::size_t>(p)])] = csr.values[static_cast<std::size_t>(p)];
    }
  }
  const pluss::Series reconstructed = pluss::make_series(
      dense_from_sparse, {csr.rows, csr.cols}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));
  const double diff = (dense - reconstructed).abs().amax(0, false).amax(0, false).item<double>();
  std::cout << "max abs diff (dense vs. reconstructed-from-sparse TF-IDF) = " << diff << "\n";

  return 0;
}
```

Output

```text
max abs diff (dense vs. reconstructed-from-sparse TF-IDF) = 0
```

### Example 14

`dump_state` / `load_state` round trip — a freshly-restored mapper
vectorizes identically to the original.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LexiconMapper original;
  original.learn({"the cat sat", "the dog ran", "cats and dogs"});
  const cls::LexiconMapperState state = original.dump_state();
  std::cout << "dumped vocabulary size = " << state.vocabulary.size() << "\n";

  cls::LexiconMapper restored;
  const cls::Status loaded = restored.load_state(state);
  std::cout << "load_state ok = " << std::boolalpha << loaded.ok() << "\n";

  const double diff = (original.map_dense({"the cat ran"}).value().x
                        - restored.map_dense({"the cat ran"}).value().x)
                           .abs().amax(0, false).amax(0, false).item<double>();
  std::cout << "max abs diff after restore = " << diff << "\n";

  return 0;
}
```

Output

```text
dumped vocabulary size = 8
load_state ok = true
max abs diff after restore = 0
```

### Example 15

An error roundup for `learn`: empty documents, `max_features <= 0`,
`min_document_frequency <= 0`, an invalid n-gram range, and a frequency
floor no term clears.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LexiconMapper empty_docs;
  std::cout << "empty documents ok      = " << std::boolalpha << empty_docs.learn({}).ok() << "\n";

  cls::LexiconMapOptions zero_features{};
  zero_features.max_features = 0;
  cls::LexiconMapper a(zero_features);
  std::cout << "max_features=0 ok       = " << a.learn({"one two"}).ok() << "\n";

  cls::LexiconMapOptions zero_dfloor{};
  zero_dfloor.min_document_frequency = 0;
  cls::LexiconMapper b(zero_dfloor);
  std::cout << "min_document_frequency=0 ok = " << b.learn({"one two"}).ok() << "\n";

  cls::LexiconMapOptions bad_ngram{};
  bad_ngram.min_ngram = 3;
  bad_ngram.max_ngram = 1;
  cls::LexiconMapper c(bad_ngram);
  std::cout << "min_ngram>max_ngram ok  = " << c.learn({"one two three"}).ok() << "\n";

  cls::LexiconMapOptions impossible_floor{};
  impossible_floor.min_document_frequency = 5;
  cls::LexiconMapper d(impossible_floor);
  const cls::Status floor_status = d.learn({"a b", "c d"});
  std::cout << "unreachable floor ok    = " << floor_status.ok() << "\n";
  std::cout << "unreachable floor message = \"" << floor_status.message << "\"\n";

  return 0;
}
```

Output

```text
empty documents ok      = false
max_features=0 ok       = false
min_document_frequency=0 ok = false
min_ngram>max_ngram ok  = false
unreachable floor ok    = false
unreachable floor message = "LexiconMapper::learn found no tokens passing min_document_frequency."
```

### Example 16

An error roundup for `load_state`: a mismatched `vocabulary`/
`document_frequency` length, and `learned = true` with an empty
`vocabulary`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LexiconMapperState mismatched{};
  mismatched.vocabulary = {"a", "b"};
  mismatched.document_frequency = {1};
  cls::LexiconMapper x;
  std::cout << "length mismatch ok       = " << std::boolalpha << x.load_state(mismatched).ok() << "\n";

  cls::LexiconMapperState empty_but_learned{};
  empty_but_learned.learned = true;
  cls::LexiconMapper y;
  std::cout << "learned=true, empty vocab ok = " << y.load_state(empty_but_learned).ok() << "\n";

  return 0;
}
```

Output

```text
length mismatch ok       = false
learned=true, empty vocab ok = false
```

## Changelog

- **0.1** — constructor, `learned`/`feature_count`/`vocabulary`, `learn`,
  `map_dense`/`map_sparse`/`map_tfidf_dense`/`map_tfidf_sparse` (each with
  a range and an initializer-list overload), `dump_state`/`load_state`.

## See Also

- [`HashMapper`](HashMapper.md) — the stateless, fixed-width counterpart.
- [`LexiconMapOptions`](LexiconMapOptions.md) /
  [`TfIdfMapOptions`](TfIdfMapOptions.md) — the two options structs this
  class reads.
- [`SparseTripletMatrix`](../sparse/SparseTripletMatrix.md) /
  [`SparseCsrMatrix`](../sparse/SparseCsrMatrix.md) — `map_sparse`/
  `map_tfidf_sparse`'s output form, and the [`sparse`](../sparse.md)
  family's own learners, which accept it directly.
- [`FeatureBatch`](../data/FeatureBatch.md) — `map_dense`/
  `map_tfidf_dense`'s output form.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
