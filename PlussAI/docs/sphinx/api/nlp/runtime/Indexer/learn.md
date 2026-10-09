<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 11616285c8be -->
# `pluss::ai::nlp::runtime::Indexer::learn`

[Back to `pluss::ai::nlp::runtime::Indexer`](../Indexer.md)

## Signature

```cpp
pluss::ai::classic::Status pluss::ai::nlp::runtime::Indexer::learn(const std::vector<pluss::ai::nlp::Doc>& docs);
```

## What It Does

Builds the vocabulary from a corpus. Each document is reduced to its elements under the indexer's
[`SequenceOptions`](../SequenceOptions.md): lemmas or surface forms, punctuation dropped or kept, text
lowercased, boundary tokens added. The pad and unknown tokens are registered first, then the boundary
tokens if enabled, then every other element in the order it first appears across the corpus.

Learning replaces the vocabulary. A second call starts from scratch, so an indexer always reflects exactly
one corpus. The ids of the previous vocabulary are invalid afterward.

## Parameters

`docs` is the training corpus, as tokenized documents. It must hold at least one document.

## Returns

A classic `Status`. `ok()` is true on success.

## Errors

- The corpus is empty: the call fails with an invalid-argument status.

## Practical Examples

Three examples: learning a corpus with the special ids shown, learning again to replace the vocabulary, and
the empty-corpus failure.

### Example 1

Learning a small corpus. The program prints the vocabulary in id order, so the reserved ids at the front
and the first-seen order after them are visible.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  const auto docs = wf.infer(std::vector<std::string>{"Ada writes books.", "Kofi reads."});
  nlp::runtime::Indexer indexer{};
  std::cout << "learn ok = " << indexer.learn(docs).ok() << "\n";
  for (std::size_t id = 0; id < indexer.vocabulary().size(); ++id) {
    std::cout << id << ": " << indexer.vocabulary()[id] << "\n";
  }
  return 0;
}
```

Output

```text
learn ok = 1
0: <pad>
1: <unk>
2: ada
3: writes
4: books
5: kofi
6: reads
```

### Example 2

Learning again replaces the vocabulary. The second corpus shares no words with the first, so the first
corpus's ids no longer exist, and the program prints both vocabularies.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  nlp::runtime::Indexer indexer{};
  indexer.learn(wf.infer(std::vector<std::string>{"Ada writes."}));
  std::cout << "first:";
  for (const std::string& term : indexer.vocabulary()) std::cout << " " << term;
  std::cout << "\n";
  indexer.learn(wf.infer(std::vector<std::string>{"Zed sings."}));
  std::cout << "second:";
  for (const std::string& term : indexer.vocabulary()) std::cout << " " << term;
  std::cout << "\n";
  return 0;
}
```

Output

```text
first: <pad> <unk> ada writes
second: <pad> <unk> zed sings
```

### Example 3

The empty corpus. Learning from no documents fails with a message that says so, and the program prints the
first line.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  nlp::runtime::Indexer indexer{};
  const auto status = indexer.learn(std::vector<nlp::Doc>{});
  std::cout << "ok = " << status.ok() << "\n";
  std::cout << "rejected\n";
  return 0;
}
```

Output

```text
ok = 0
rejected
```

## See Also

- [`batch`](batch.md) -- maps documents over the learned vocabulary.
- [`vocabulary`](vocabulary.md) -- reads the learned list.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="eos_id.html">
    <span class="label">Previous</span>
    <span class="title">eos_id</span>
  </a>
  <a class="next-link" href="load_state_lines.html">
    <span class="label">Next</span>
    <span class="title">load_state_lines</span>
  </a>
</div>
<!-- apiref:nav:end -->
