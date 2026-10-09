<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 18c3b4b9f496 -->
# `pluss::ai::nlp::runtime::Indexer::vocabulary`

[Back to `pluss::ai::nlp::runtime::Indexer`](../Indexer.md)

## Signature

```cpp
const std::vector<std::string>& pluss::ai::nlp::runtime::Indexer::vocabulary() const noexcept;
```

## What It Does

Returns the vocabulary in id order: element `i` of the vector is the element with id `i`. The first entries
are the pad and unknown tokens, followed by any boundary tokens, then the corpus elements in first-seen
order.

## Parameters

None.

## Returns

A `const` reference to the vocabulary. It is empty for an indexer that has not learned or loaded state. The
reference stays valid until the next `learn` or `load_state_lines` call.

## Errors

None. The method is `noexcept`.

## Practical Examples

Three examples: the vocabulary of a learned indexer, the empty vocabulary of a new one, and the id of a word
looked up in the vocabulary.

### Example 1

The learned vocabulary, printed with each id, showing the reserved entries at the front.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  nlp::runtime::Indexer indexer{};
  indexer.learn(wf.infer(std::vector<std::string>{"Ada writes books."}));
  for (std::size_t id = 0; id < indexer.vocabulary().size(); ++id) {
    std::cout << id << ": " << indexer.vocabulary()[id] << "\n";
  }
  return 0;
}
```

Output

```text
0: <pad>
1: <unk>
2: ada
3: writes
4: books
```

### Example 2

A new indexer has no vocabulary, which is how a caller tells an unlearned indexer from a learned one.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  const nlp::runtime::Indexer indexer{};
  std::cout << "size = " << indexer.vocabulary().size() << ", empty = " << indexer.vocabulary().empty() << "\n";
  return 0;
}
```

Output

```text
size = 0, empty = 1
```

### Example 3

Finding an element's id by scanning the vocabulary, which is the same id the batch uses for that element.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  nlp::runtime::Indexer indexer{};
  indexer.learn(wf.infer(std::vector<std::string>{"Ada writes books."}));
  const auto& vocab = indexer.vocabulary();
  const auto found = std::find(vocab.begin(), vocab.end(), "books");
  std::cout << "books has id " << (found - vocab.begin()) << "\n";
  return 0;
}
```

Output

```text
books has id 4
```

## See Also

- [`learn`](learn.md) -- builds this vocabulary.
- [`pad_id`](pad_id.md) and [`unk_id`](unk_id.md) -- the reserved ids.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="unk_id.html">
    <span class="label">Previous</span>
    <span class="title">unk_id</span>
  </a>
</div>
<!-- apiref:nav:end -->
