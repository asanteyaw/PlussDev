# `pluss::ai::classic::HashMapper`

A stateless hashing vectorizer: every n-gram is mapped directly to a fixed
output column via a hash function, with no learned vocabulary at all — no
`learn()` step, no `feature_count()` that changes with the data, no term
ever mapped back from a column index. The counterpart of
[`LexiconMapper`](LexiconMapper.md) for the case an unbounded or unknown-
ahead-of-time vocabulary makes a real vocabulary impractical.

## Signature

```cpp
explicit classic::HashMapper::HashMapper(classic::HashMapOptions options = {});

const classic::HashMapOptions& classic::HashMapper::options() const noexcept;

classic::StatusOr<classic::FeatureBatch> classic::HashMapper::map_dense(
    /* a range of std::string -- pass a std::vector<std::string> or braced list */ documents) const;
classic::StatusOr<classic::SparseTripletMatrix> classic::HashMapper::map_sparse(
    /* a range of std::string */ documents) const;
```

`map_dense` and `map_sparse` each also have a second overload taking
`std::initializer_list<std::string>` in place of the range — the common
case when documents are spelled directly as a braced `{...}` list. Both
forms are exercised in the examples below.

## Construction

`HashMapper(options = {})` takes a [`HashMapOptions`](HashMapOptions.md)
and never fails, exactly like [`LexiconMapper`](LexiconMapper.md)'s own
constructor — but unlike `LexiconMapper`, **there is no `learn()` step at
all** that could validate `options` up front. Every field
([`n_features`](HashMapOptions.md#n_features), the n-gram range) is
instead validated fresh on **every** `map_dense`/`map_sparse` call — a
`HashMapper` built with `n_features = 0` constructs cleanly, and every
subsequent `map_*` call on it fails identically. See
[Example 7](#example-7).

## State

`options()` — `const HashMapOptions&`
: The exact [`HashMapOptions`](HashMapOptions.md) this instance was
  constructed with, read back unchanged.

There is no `learned()`/`trained()` state — a `HashMapper` is ready to map
documents the moment it is constructed.

## Methods

`map_dense(documents)`
: `[rows, options().n_features]` — every hashed n-gram's column is
  `hash(term) % n_features`; a column accumulates every term that hashes
  there (see [`HashMapOptions::binary`/`alternate_sign`](HashMapOptions.md)
  for exactly how). The output width is always `n_features`, regardless of
  how many distinct terms actually appeared — unlike
  [`LexiconMapper::map_dense`](LexiconMapper.md#methods), whose width is
  the learned vocabulary size.

`map_sparse(documents)`
: The same values as `map_dense`, as a
  [`SparseTripletMatrix`](../sparse/SparseTripletMatrix.md) instead —
  proven numerically identical in [Example 2](#example-2). Like
  [`LexiconMapper::map_sparse`](LexiconMapper.md#methods), each row's
  nonzero columns are accumulated in a hash map first, so no `(row,
  column)` pair is ever duplicated here.

## Errors

- `map_dense` / `map_sparse`: `documents` empty —
  `Status::invalid_argument`.
- `map_dense` / `map_sparse`: `options().n_features <= 0` —
  `Status::invalid_argument`.
- `map_dense` / `map_sparse`: an invalid n-gram range (either `<= 0`, or
  `min_ngram > max_ngram`) — `Status::invalid_argument`.

## Practical Examples

A basic lifecycle (no `learn()` needed), the two output forms proven
numerically identical, a deliberately forced hash collision,
`alternate_sign`'s sign flip, `binary` combined with `alternate_sign`
producing a genuine `-1.0` (not just `0`/`1`), bigrams via `min_ngram`/
`max_ngram`, an error roundup proving validation runs on every call (not
once), determinism across independent instances with identical options,
and a fixed-width comparison against `LexiconMapper`'s growing vocabulary.

### Example 1

Two short documents, a small `n_features` so the whole matrix is legible
— no `learn()` call anywhere.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::HashMapOptions options{};
  options.n_features = 16;
  cls::HashMapper mapper(options);

  const cls::FeatureBatch batch = mapper.map_dense({"the cat sat", "the dog ran"}).value();
  std::cout << "shape = [" << batch.x.sizes()[0] << ", " << batch.x.sizes()[1] << "]\n";
  std::cout << "matrix = " << batch.x << "\n";

  return 0;
}
```

Output

```text
shape = [2, 16]
matrix = pluss::Series(shape=[2,16], strides=[16,1], dtype=float64, platform=cpu, numel=32, contiguous=true)
data=[[0, 0, ... , 0],
  [0, 0, ... , 0]]
```

### Example 2

`map_sparse` proven numerically identical to `map_dense`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::HashMapOptions options{};
  options.n_features = 64;
  cls::HashMapper mapper(options);

  const cls::FeatureBatch dense = mapper.map_dense({"the cat sat", "the dog ran fast"}).value();
  const cls::SparseTripletMatrix sparse = mapper.map_sparse({"the cat sat", "the dog ran fast"}).value();
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

A tiny `n_features` deliberately forces real hash collisions — of the six
terms tried, `"cat"`/`"dog"`/`"bird"`/`"fish"` all land in the same
column, and `"lion"`/`"wolf"` both land in a second column; the third
column is never hit by any of the six.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::HashMapOptions options{};
  options.n_features = 3;
  cls::HashMapper mapper(options);

  for (const std::string& term : {"cat", "dog", "bird", "fish", "lion", "wolf"}) {
    const pluss::Series row = mapper.map_dense({term}).value().x;
    std::cout << term << " -> " << row << "\n";
  }

  return 0;
}
```

Output

```text
cat -> pluss::Series(shape=[1,3], strides=[3,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[[0, 0, 1]]
dog -> pluss::Series(shape=[1,3], strides=[3,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[[0, 0, 1]]
bird -> pluss::Series(shape=[1,3], strides=[3,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[[0, 0, 1]]
fish -> pluss::Series(shape=[1,3], strides=[3,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[[0, 0, 1]]
lion -> pluss::Series(shape=[1,3], strides=[3,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[[1, 0, 0]]
wolf -> pluss::Series(shape=[1,3], strides=[3,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[[1, 0, 0]]
```

### Example 4

`alternate_sign` flips some terms' contribution negative — off by default,
every term contributes positively. Of these four terms, only `"cat"`'s
hash happens to have its top bit set, so only `"cat"`'s column reads
`-1.0` under `alternate_sign`; the other three are unaffected. Printed as
a plain vector (rather than through `Series::operator<<`, which truncates
a row this wide) so every column is visible.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

namespace {
void print_row(const std::string& label, const pluss::Series& row) {
  const std::vector<double> vec = row.to(pluss::pCPU, pluss::ScalarType::Float64).contiguous().to_vec<double>();
  std::cout << label << ": [";
  for (double v : vec) std::cout << v << " ";
  std::cout << "]\n";
}
}  // namespace

int main() {
  namespace cls = pluss::ai::classic;

  cls::HashMapOptions options{};
  options.n_features = 8;
  cls::HashMapper positive_only(options);

  options.alternate_sign = true;
  cls::HashMapper signed_mapper(options);

  for (const std::string& term : {"cat", "dog", "bird", "fish"}) {
    print_row(term + " no-alternate-sign", positive_only.map_dense({term}).value().x);
    print_row(term + " alternate-sign   ", signed_mapper.map_dense({term}).value().x);
  }

  return 0;
}
```

Output

```text
cat no-alternate-sign: [0 0 0 0 0 1 0 0 ]
cat alternate-sign   : [0 0 0 0 0 -1 0 0 ]
dog no-alternate-sign: [0 0 0 1 0 0 0 0 ]
dog alternate-sign   : [0 0 0 1 0 0 0 0 ]
bird no-alternate-sign: [1 0 0 0 0 0 0 0 ]
bird alternate-sign   : [1 0 0 0 0 0 0 0 ]
fish no-alternate-sign: [0 1 0 0 0 0 0 0 ]
fish alternate-sign   : [0 1 0 0 0 0 0 0 ]
```

### Example 5

`binary = true` with `alternate_sign = true` together: `"cat"`'s column
reads exactly `-1.0`, not just `0.0`/`1.0` — genuinely different from
[`LexiconMapOptions::binary`](LexiconMapOptions.md#binary), which always
sets `1.0`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

namespace {
void print_row(const std::string& label, const pluss::Series& row) {
  const std::vector<double> vec = row.to(pluss::pCPU, pluss::ScalarType::Float64).contiguous().to_vec<double>();
  std::cout << label << ": [";
  for (double v : vec) std::cout << v << " ";
  std::cout << "]\n";
}
}  // namespace

int main() {
  namespace cls = pluss::ai::classic;

  cls::HashMapOptions options{};
  options.n_features = 8;
  options.binary = true;
  options.alternate_sign = true;
  cls::HashMapper mapper(options);

  for (const std::string& term : {"cat", "dog", "bird", "fish"}) {
    print_row(term, mapper.map_dense({term}).value().x);
  }

  return 0;
}
```

Output

```text
cat: [0 0 0 0 0 -1 0 0 ]
dog: [0 0 0 1 0 0 0 0 ]
bird: [1 0 0 0 0 0 0 0 ]
fish: [0 1 0 0 0 0 0 0 ]
```

### Example 6

`min_ngram = max_ngram = 2` (bigrams only) — same n-gram mechanics as
[`LexiconMapOptions`](LexiconMapOptions.md#min_ngram). `"the cat sat"`'s
three unigram tokens hash to only two distinct columns here (a genuine
collision at this `n_features`, one column reading `2.0`); its two
bigrams (`"the cat"`, `"cat sat"`) hash to two different columns, no
collision between them.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

namespace {
void print_row(const std::string& label, const pluss::Series& row) {
  const std::vector<double> vec = row.to(pluss::pCPU, pluss::ScalarType::Float64).contiguous().to_vec<double>();
  std::cout << label << ": [";
  for (double v : vec) std::cout << v << " ";
  std::cout << "]\n";
}
}  // namespace

int main() {
  namespace cls = pluss::ai::classic;

  cls::HashMapOptions unigram{};
  unigram.n_features = 16;
  cls::HashMapper unigram_mapper(unigram);

  cls::HashMapOptions bigram{};
  bigram.n_features = 16;
  bigram.min_ngram = 2;
  bigram.max_ngram = 2;
  cls::HashMapper bigram_mapper(bigram);

  const std::string doc = "the cat sat";
  print_row("unigram", unigram_mapper.map_dense({doc}).value().x);
  print_row("bigram ", bigram_mapper.map_dense({doc}).value().x);

  return 0;
}
```

Output

```text
unigram: [0 0 0 0 0 2 1 0 0 0 0 0 0 0 0 0 ]
bigram : [0 1 0 0 0 0 1 0 0 0 0 0 0 0 0 0 ]
```

### Example 7

Validation runs on **every** `map_dense`/`map_sparse` call, not once at
construction — a `HashMapper` built with `n_features = 0` constructs
cleanly, and two independent calls on it both fail identically.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::HashMapOptions bad{};
  bad.n_features = 0;
  cls::HashMapper mapper(bad);
  std::cout << "construction always succeeds -- no crash reaching this line\n";

  std::cout << "first map_dense call ok  = " << std::boolalpha << mapper.map_dense({"hello"}).ok() << "\n";
  std::cout << "second map_dense call ok = " << mapper.map_dense({"hello again"}).ok() << "\n";

  cls::HashMapOptions bad_ngram{};
  bad_ngram.min_ngram = 5;
  bad_ngram.max_ngram = 1;
  cls::HashMapper ngram_mapper(bad_ngram);
  std::cout << "bad ngram range ok       = " << ngram_mapper.map_dense({"hello"}).ok() << "\n";

  cls::HashMapper empty_docs_mapper;
  std::cout << "empty documents ok       = " << empty_docs_mapper.map_dense({}).ok() << "\n";

  return 0;
}
```

Output

```text
construction always succeeds -- no crash reaching this line
first map_dense call ok  = false
second map_dense call ok = false
bad ngram range ok       = false
empty documents ok       = false
```

### Example 8

Two independent `HashMapper` instances built with identical options
produce byte-identical output — purely a function of `options()` and the
input, no hidden mutable state.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::HashMapOptions options{};
  options.n_features = 32;
  options.alternate_sign = true;

  cls::HashMapper first(options);
  cls::HashMapper second(options);

  const double diff = (first.map_dense({"the cat sat", "the dog ran"}).value().x
                        - second.map_dense({"the cat sat", "the dog ran"}).value().x)
                           .abs().amax(0, false).amax(0, false).item<double>();
  std::cout << "max abs diff between two independent instances = " << diff << "\n";

  return 0;
}
```

Output

```text
max abs diff between two independent instances = 0
```

### Example 9

The fixed-width contrast with [`LexiconMapper`](LexiconMapper.md): mapping
an ever-growing set of distinct documents never changes `HashMapper`'s
output width, unlike a learned vocabulary.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::HashMapOptions options{};
  options.n_features = 16;
  cls::HashMapper mapper(options);

  std::cout << "3 documents -> width = " << mapper.map_dense({"a", "b", "c"}).value().x.sizes()[1] << "\n";
  std::cout << "30 distinct-word documents -> width = "
            << mapper.map_dense({"aardvark", "bear", "cat", "dog", "eel", "fox", "goat", "hare",
                                  "ibis", "jay", "koala", "lynx", "mole", "newt", "owl", "pig",
                                  "quail", "rat", "seal", "toad", "urchin", "vole", "wolf", "yak",
                                  "zebu", "ant", "bee", "cow", "doe", "emu"})
                   .value()
                   .x.sizes()[1]
            << " (unchanged, unlike LexiconMapper::feature_count())\n";

  return 0;
}
```

Output

```text
3 documents -> width = 16
30 distinct-word documents -> width = 16 (unchanged, unlike LexiconMapper::feature_count())
```

## Changelog

- **0.1** — constructor, `options`, `map_dense`/`map_sparse` (each with a
  range and an initializer-list overload).

## See Also

- [`LexiconMapper`](LexiconMapper.md) — the vocabulary-backed counterpart.
- [`HashMapOptions`](HashMapOptions.md) — the options struct this class
  reads fresh on every call.
- [`SparseTripletMatrix`](../sparse/SparseTripletMatrix.md) /
  [`SparseCsrMatrix`](../sparse/SparseCsrMatrix.md) — `map_sparse`'s output
  form, and the [`sparse`](../sparse.md) family's own learners, which
  accept it directly.
- [`FeatureBatch`](../data/FeatureBatch.md) — `map_dense`'s output form.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
