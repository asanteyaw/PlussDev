<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 9472e54c14c4 -->
# `pluss::ai::nlp::runtime::Indexer::load_state_lines`

[Back to `pluss::ai::nlp::runtime::Indexer`](../Indexer.md)

## Signature

```cpp
pluss::ai::classic::Status pluss::ai::nlp::runtime::Indexer::load_state_lines(pluss::RangeView<std::string_view> lines);
```

## What It Does

Restores the indexer from lines written by [`dump_state_lines`](dump_state_lines.md). The options, the
reserved ids, and the vocabulary are all replaced by the saved values. A restored indexer batches documents
exactly as the original did.

Loading checks the lines before it changes anything. A missing vocabulary entry, a count that does not match
the entries, or a line it does not recognize is rejected, and the indexer keeps its previous state.

## Parameters

`lines` is the saved state, one entry per line, in the order `dump_state_lines` wrote them.

## Returns

A classic `Status`. `ok()` is true when the state was restored.

## Errors

- The lines are empty: the call fails with an invalid-argument status.
- A line is not recognized, or the vocabulary count does not match the vocabulary entries: the call fails
  with a message that names the problem.

## Practical Examples

Three examples: a round trip through the saved lines, a rejected state, and the restored indexer batching a
document.

### Example 1

A round trip. The saved lines restore a fresh indexer with the same vocabulary, which the program prints.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  nlp::runtime::Indexer original{};
  original.learn(wf.infer(std::vector<std::string>{"Ada writes."}));
  const auto lines = original.dump_state_lines();
  const std::vector<std::string_view> views(lines.begin(), lines.end());
  nlp::runtime::Indexer restored{};
  std::cout << "load ok = " << restored.load_state_lines(views).ok() << "\n";
  std::cout << "vocabulary size = " << restored.vocabulary().size() << "\n";
  return 0;
}
```

Output

```text
load ok = 1
vocabulary size = 4
```

### Example 2

A rejected state. A line the loader does not recognize is refused, and the program prints the first line of
the message.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  nlp::runtime::Indexer indexer{};
  const std::vector<std::string_view> bad = {"not_a_key=1"};
  const auto status = indexer.load_state_lines(bad);
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

### Example 3

The restored indexer batches a new document. Its unknown word maps to the same id the original would give it.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  nlp::runtime::Indexer original{};
  original.learn(wf.infer(std::vector<std::string>{"Ada writes."}));
  const auto lines = original.dump_state_lines();
  const std::vector<std::string_view> views(lines.begin(), lines.end());
  nlp::runtime::Indexer restored{};
  restored.load_state_lines(views);
  const auto batch = restored.batch(wf.infer(std::vector<std::string>{"Kofi writes."})).value();
  const auto ids = batch.token_ids.to(pluss::pCPU, pluss::ScalarType::Int64).contiguous().template to_vec<std::int64_t>();
  std::cout << "Kofi maps to unk = " << (ids[0] == restored.unk_id()) << "\n";
  return 0;
}
```

Output

```text
Kofi maps to unk = 1
```

## See Also

- [`dump_state_lines`](dump_state_lines.md) -- writes the lines this method reads.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="learn.html">
    <span class="label">Previous</span>
    <span class="title">learn</span>
  </a>
  <a class="next-link" href="options.html">
    <span class="label">Next</span>
    <span class="title">options</span>
  </a>
</div>
<!-- apiref:nav:end -->
