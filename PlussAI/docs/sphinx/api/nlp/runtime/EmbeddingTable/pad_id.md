<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 833620f6ef37 -->
# `pluss::ai::nlp::runtime::EmbeddingTable::pad_id`

[Back to `pluss::ai::nlp::runtime::EmbeddingTable`](../EmbeddingTable.md)

## Signature

```cpp
std::int64_t pluss::ai::nlp::runtime::EmbeddingTable::pad_id() const noexcept;
```

## What It Does

Returns the id whose row the table zeroed at reset, normally the indexer's pad id.

## Parameters

None.

## Returns

The pad id, or `-1` when no pad row was set.

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
  std::cout << "pad_id = " << table.pad_id() << "\n";
  return 0;
}
```

Output

```text
pad_id = 0
```

### Example 2

The value on a new table, before reset.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  const nlp::runtime::EmbeddingTable table{};
  std::cout << "pad_id before reset = " << table.pad_id() << "\n";
  return 0;
}
```

Output

```text
pad_id before reset = -1
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
  std::cout << "restored pad_id is available\n";
  return 0;
}
```

Output

```text
restored pad_id is available
```

## See Also

- [`EmbeddingTable`](../EmbeddingTable.md) -- the table.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="options.html">
    <span class="label">Previous</span>
    <span class="title">options</span>
  </a>
  <a class="next-link" href="position_weights.html">
    <span class="label">Next</span>
    <span class="title">position_weights</span>
  </a>
</div>
<!-- apiref:nav:end -->
