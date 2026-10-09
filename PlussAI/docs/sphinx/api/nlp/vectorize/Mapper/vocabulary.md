<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: cf69a8dd888b -->
# `pluss::ai::nlp::vectorize::Mapper::vocabulary`

[Back to `pluss::ai::nlp::vectorize::Mapper`](../Mapper.md)

## Signature

```cpp
const std::vector<std::string>& pluss::ai::nlp::vectorize::Mapper::vocabulary() const noexcept;
```

## What It Does

Returns the learned vocabulary: the terms in column order, so `vocabulary()[i]` names column `i` of
every dense and sparse matrix this mapper produces. The order is the one [`learn`](learn.md) fixed,
by document frequency and then alphabetically, after the `min_document_frequency` filter and the
`max_features` cap.

An untrained mapper has an empty vocabulary. The reference stays valid until the next `learn` or
`load_state` call on the same mapper, so copy it before retraining if it must outlive that call.

## Parameters

None.

## Returns

A `const` reference to the vocabulary. Its length is the feature width of the mapper's output.

## Errors

None. The method is `noexcept` and never fails; an untrained mapper returns an empty vector.

## Practical Examples

Three examples: reading the column names of a learned mapper, the empty vocabulary of an untrained
mapper, and the vocabulary after a cap.

### Example 1

The column names of a learned mapper, printed with their index, so the column order used by every
matrix is explicit.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize()}};
  nlp::vectorize::Mapper mapper{};
  mapper.learn(wf.infer(std::vector<std::string>{"Ada wrote books.", "Kofi writes books."}));

  const auto& vocabulary = mapper.vocabulary();
  for (std::size_t column = 0; column < vocabulary.size(); ++column) {
    std::cout << "column " << column << " = " << vocabulary[column] << "\n";
  }
  return 0;
}
```

Output

```text
column 0 = book
column 1 = write
column 2 = ada
column 3 = kofi
```

### Example 2

An untrained mapper. Its vocabulary is empty, which is the signal that it cannot map anything yet.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  const nlp::vectorize::Mapper mapper{};
  std::cout << "vocabulary size = " << mapper.vocabulary().size() << "\n";
  std::cout << "empty = " << mapper.vocabulary().empty() << "\n";
  return 0;
}
```

Output

```text
vocabulary size = 0
empty = 1
```

### Example 3

The vocabulary after a cap. With `max_features` set to two, only the two most frequent terms survive,
and the program prints them so the cut is visible.

```cpp
#include <pluss/nlp.h>
#include <pluss/ai/ml.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  const auto docs = wf.infer(std::vector<std::string>{"cat sat", "dog sat", "cat cat"});
  nlp::vectorize::Mapper mapper{pluss::ai::classic::LexiconMapOptions{.max_features = 2}};
  mapper.learn(docs);

  std::cout << "vocabulary:";
  for (const std::string& term : mapper.vocabulary()) std::cout << " " << term;
  std::cout << "\n";
  return 0;
}
```

Output

```text
vocabulary: cat sat
```

## See Also

- [`learn`](learn.md) -- fixes the vocabulary this method returns.
- [`dump_state`](dump_state.md) -- the vocabulary within the full state.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tfidf_sparse.html">
    <span class="label">Previous</span>
    <span class="title">tfidf_sparse</span>
  </a>
</div>
<!-- apiref:nav:end -->
