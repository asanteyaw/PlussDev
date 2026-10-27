<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 3c1048994560 -->
# `pluss::ai::nlp::vectorize::Mapper::load_state`

[Back to `pluss::ai::nlp::vectorize::Mapper`](../Mapper.md)

## Signature

```cpp
pluss::ai::classic::Status pluss::ai::nlp::vectorize::Mapper::load_state(
    const pluss::ai::classic::LexiconMapperState& state);
```

## What It Does

Replaces the mapper's state with a previously captured one, so the mapper maps documents the way the
original did. The state's options, vocabulary, and document frequencies all come from the saved
state, and the mapper's own `DocMapOptions` are kept, because the state does not carry them.

Loading validates the state before it is installed. A state whose vocabulary cap or document
frequency threshold is not positive, or whose n-gram range is invalid, is rejected and the mapper keeps
its previous state. A failed load therefore never leaves a half-restored mapper.

## Parameters

`state` is a `LexiconMapperState`, normally the value [`dump_state`](dump_state.md) returned or one
read back from storage.

## Returns

A classic `Status`. It reports success with `ok()`. On rejection it carries a message that names the
invalid option.

## Errors

- A non-positive `max_features` or `min_document_frequency` in the state's options.
- An invalid n-gram range in the state's options.

## Practical Examples

Three examples: restoring a state into a fresh mapper, a rejected state that leaves the mapper
unchanged, and a restored mapper mapping an unseen document.

### Example 1

Restoring a state into a fresh mapper. The restored vocabulary matches the original's, term for term.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  nlp::vectorize::Mapper original{};
  original.learn(wf.infer(std::vector<std::string>{"cat sat", "dog sat", "cat cat"}));

  nlp::vectorize::Mapper restored{};
  const auto status = restored.load_state(original.dump_state());
  std::cout << "load ok = " << status.ok() << "\n";
  std::cout << "restored vocabulary:";
  for (const std::string& term : restored.vocabulary()) std::cout << " " << term;
  std::cout << "\n";
  return 0;
}
```

Output

```text
load ok = 1
restored vocabulary: cat sat dog
```

### Example 2

A rejected state. A state whose vocabulary cap is zero is refused, and the mapper keeps the vocabulary
it already had. The program prints the first line of the message.

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
  mapper.learn(wf.infer(std::vector<std::string>{"cat sat"}));

  pluss::ai::classic::LexiconMapperState bad = mapper.dump_state();
  bad.options.max_features = 0;
  const auto status = mapper.load_state(bad);
  std::cout << "load ok = " << status.ok() << "\n";
  std::cout << "vocabulary kept:";
  for (const std::string& term : mapper.vocabulary()) std::cout << " " << term;
  std::cout << "\n";
  return 0;
}
```

Output

```text
load ok = 0
vocabulary kept: cat sat
```

### Example 3

A restored mapper mapping a document it never saw. The restored vocabulary drops the unseen word and
keeps the known one, exactly as the original would.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  nlp::vectorize::Mapper original{};
  original.learn(wf.infer(std::vector<std::string>{"cat sat", "dog sat"}));

  nlp::vectorize::Mapper restored{};
  restored.load_state(original.dump_state());
  const auto batch = restored.count_dense(wf.infer(std::vector<std::string>{"zebra cat"})).value();
  const auto values = batch.x.to(pluss::pCPU, pluss::ScalarType::Float64).contiguous().template host<double>();
  std::cout << "row shape = " << batch.sample_count() << " x " << batch.feature_count() << "\n";
  std::cout << "counts:";
  for (const double value : values) std::cout << " " << value;
  std::cout << "\n";
  return 0;
}
```

Output

```text
row shape = 1 x 3
counts: 0 1 0
```

## See Also

- [`dump_state`](dump_state.md) -- captures the state this method restores.
- [`learn`](learn.md) -- builds a state from a corpus instead.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="learn.html">
    <span class="label">Previous</span>
    <span class="title">learn</span>
  </a>
  <a class="next-link" href="tfidf_dense.html">
    <span class="label">Next</span>
    <span class="title">tfidf_dense</span>
  </a>
</div>
<!-- apiref:nav:end -->
