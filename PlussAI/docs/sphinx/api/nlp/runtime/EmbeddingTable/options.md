<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 88ba5422edb6 -->
# `pluss::ai::nlp::runtime::EmbeddingTable::options`

[Back to `pluss::ai::nlp::runtime::EmbeddingTable`](../EmbeddingTable.md)

## Signature

```cpp
const pluss::ai::nlp::runtime::EmbeddingOptions& pluss::ai::nlp::runtime::EmbeddingTable::options() const noexcept;
```

## What It Does

Returns the options the table was constructed or restored with. They define the width, the position policy, the numeric type, and the seed of the table.

## Parameters

None.

## Returns

A `const` reference to the [`EmbeddingOptions`](../EmbeddingOptions.md), valid for the life of the table.

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
  std::cout << "width = " << table.options().width << ", use_positions = " << table.options().use_positions << "\n";
  return 0;
}
```

Output

```text
width = 4, use_positions = 1
```

### Example 2

The value on a new table, before reset.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  const nlp::runtime::EmbeddingTable table{};
  std::cout << "width before reset = " << table.options().width << "\n";
  return 0;
}
```

Output

```text
width before reset = 128
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
  std::cout << "restored options is available\n";
  return 0;
}
```

Output

```text
restored options is available
```

## See Also

- [`EmbeddingTable`](../EmbeddingTable.md) -- the table.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="load_state_lines.html">
    <span class="label">Previous</span>
    <span class="title">load_state_lines</span>
  </a>
  <a class="next-link" href="pad_id.html">
    <span class="label">Next</span>
    <span class="title">pad_id</span>
  </a>
</div>
<!-- apiref:nav:end -->
