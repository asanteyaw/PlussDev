<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 09f631ecd363 -->
# `pluss::ai::nlp::vectorize::Mapper::dump_state`

[Back to `pluss::ai::nlp::vectorize::Mapper`](../Mapper.md)

## Signature

```cpp
pluss::ai::classic::LexiconMapperState pluss::ai::nlp::vectorize::Mapper::dump_state() const;
```

## What It Does

Captures the mapper's complete state as a `LexiconMapperState`: its options, whether it has been
learned, the number of documents it learned from, the vocabulary, and each term's document frequency.
Restoring that state with [`load_state`](load_state.md) rebuilds a mapper that produces the same
matrices, so a trained vectorizer can be saved next to a trained model.

The state does not include the `DocMapOptions` the mapper was built with, because those options are
not part of the classic mapper. Save them alongside the state when the restored mapper must use the
same feature-text rule.

## Parameters

None.

## Returns

A `LexiconMapperState` with the fields `options`, `learned`, `learned_document_count`, `vocabulary`,
and `document_frequency`. The fields are plain data and can be written to any storage.

## Errors

None. The method is `const` and does not fail; an untrained mapper returns a state with
`learned` set to false and empty vocabulary.

## Practical Examples

Three examples: the state of a learned mapper, the state of an untrained one, and a round trip that
produces the same counts.

### Example 1

The fields of a learned mapper's state, printed so the stored vocabulary and document frequencies can
be read against the corpus.

```cpp
#include <pluss/nlp.h>
#include <pluss/ai/ml.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  nlp::vectorize::Mapper mapper{};
  mapper.learn(wf.infer(std::vector<std::string>{"cat sat", "dog sat", "cat cat"}));

  const pluss::ai::classic::LexiconMapperState state = mapper.dump_state();
  std::cout << "learned = " << state.learned << ", learned_document_count = " << state.learned_document_count << "\n";
  for (std::size_t i = 0; i < state.vocabulary.size(); ++i) {
    std::cout << state.vocabulary[i] << ": df = " << state.document_frequency[i] << "\n";
  }
  return 0;
}
```

Output

```text
learned = 1, learned_document_count = 3
cat: df = 2
sat: df = 2
dog: df = 1
```

### Example 2

The state of an untrained mapper. It reports that nothing was learned and carries no vocabulary, so a
caller can tell a fresh mapper from one that holds a model.

```cpp
#include <pluss/nlp.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  const nlp::vectorize::Mapper mapper{};
  const pluss::ai::classic::LexiconMapperState state = mapper.dump_state();
  std::cout << "learned = " << state.learned << ", vocabulary size = " << state.vocabulary.size() << "\n";
  return 0;
}
```

Output

```text
learned = 0, vocabulary size = 0
```

### Example 3

A round trip. The state of a trained mapper is restored into a fresh mapper, which then produces the
same counts as the original for the same documents.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  const auto docs = wf.infer(std::vector<std::string>{"cat sat", "dog sat", "cat cat"});
  nlp::vectorize::Mapper original{};
  original.learn(docs);

  nlp::vectorize::Mapper restored{};
  restored.load_state(original.dump_state());

  const auto a = original.count_dense(docs).value();
  const auto b = restored.count_dense(docs).value();
  const auto av = a.x.to(pluss::pCPU, pluss::ScalarType::Float64).contiguous().template to_vec<double>();
  const auto bv = b.x.to(pluss::pCPU, pluss::ScalarType::Float64).contiguous().template to_vec<double>();
  std::cout << "same shape = " << (a.feature_count() == b.feature_count()) << "\n";
  std::cout << "same counts = " << (av == bv) << "\n";
  return 0;
}
```

Output

```text
same shape = 1
same counts = 1
```

## See Also

- [`load_state`](load_state.md) -- restores the state this method captures.
- [`vocabulary`](vocabulary.md) -- reads the vocabulary alone.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="count_sparse.html">
    <span class="label">Previous</span>
    <span class="title">count_sparse</span>
  </a>
  <a class="next-link" href="learn.html">
    <span class="label">Next</span>
    <span class="title">learn</span>
  </a>
</div>
<!-- apiref:nav:end -->
