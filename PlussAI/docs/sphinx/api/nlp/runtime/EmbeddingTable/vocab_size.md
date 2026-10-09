<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: dbeae4ae1ee7 -->
# `pluss::ai::nlp::runtime::EmbeddingTable::vocab_size`

[Back to `pluss::ai::nlp::runtime::EmbeddingTable`](../EmbeddingTable.md)

## Signature

```cpp
std::int64_t pluss::ai::nlp::runtime::EmbeddingTable::vocab_size() const noexcept;
```

## What It Does

Returns the number of ids the table was reset to hold, which is the number of rows of the token weights.

## Parameters

None.

## Returns

The vocabulary size, or zero before the table is reset or loaded.

## Errors

None. The method is `noexcept`.

## Practical Examples

Three examples: the value on a reset table, the value on a new table, and the value after restoring state.

### Example 1

The value on a table reset for the indexer's vocabulary.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  nlp::runtime::Indexer indexer{};
  indexer.learn(wf.infer(std::vector<std::string>{"Ada writes books.", "Kofi reads."}));
  const nlp::runtime::SequenceBatch batch = indexer.batch(wf.infer(std::vector<std::string>{"Ada reads.", "Kofi."})).value();
  nlp::runtime::EmbeddingTable table{nlp::runtime::EmbeddingOptions{.width = 4}};
  table.reset(static_cast<std::int64_t>(indexer.vocabulary().size()), indexer.pad_id());
  std::cout << "vocab_size = " << table.vocab_size() << "\n";
  return 0;
}
```

Output

```text
vocab_size = 7
```

### Example 2

The value on a new table, before reset.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  const nlp::runtime::EmbeddingTable table{};
  std::cout << "vocab_size before reset = " << table.vocab_size() << "\n";
  return 0;
}
```

Output

```text
vocab_size before reset = 0
```

### Example 3

The value after restoring a saved table.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  nlp::runtime::Indexer indexer{};
  indexer.learn(wf.infer(std::vector<std::string>{"Ada writes books.", "Kofi reads."}));
  const nlp::runtime::SequenceBatch batch = indexer.batch(wf.infer(std::vector<std::string>{"Ada reads.", "Kofi."})).value();
  nlp::runtime::EmbeddingTable original{nlp::runtime::EmbeddingOptions{.width = 4}};
  original.reset(static_cast<std::int64_t>(indexer.vocabulary().size()), indexer.pad_id());
  const auto lines = original.dump_state_lines();
  const std::vector<std::string_view> views(lines.begin(), lines.end());
  nlp::runtime::EmbeddingTable restored{};
  restored.load_state_lines(views);
  std::cout << "restored vocab_size is available\n";
  return 0;
}
```

Output

```text
restored vocab_size is available
```

## See Also

- [`EmbeddingTable`](../EmbeddingTable.md) -- the table.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="reset.html">
    <span class="label">Previous</span>
    <span class="title">reset</span>
  </a>
  <a class="next-link" href="weights.html">
    <span class="label">Next</span>
    <span class="title">weights</span>
  </a>
</div>
<!-- apiref:nav:end -->
