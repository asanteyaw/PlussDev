<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6f2928fe782f -->
# `pluss::ai::nlp::runtime::EmbeddingTable::position_weights`

[Back to `pluss::ai::nlp::runtime::EmbeddingTable`](../EmbeddingTable.md)

## Signature

```cpp
const pluss::Series& pluss::ai::nlp::runtime::EmbeddingTable::position_weights() const noexcept;
```

## What It Does

Returns the position weight matrix, shaped `[max_positions, width]`, or an undefined series when positions are off.

## Parameters

None.

## Returns

A `const` reference to the position weights. It is undefined when positions are off or the table is not reset.

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
  std::cout << "position weights defined = " << table.position_weights().defined() << "\n";
  return 0;
}
```

Output

```text
position weights defined = 1
```

### Example 2

The value on a new table, before reset.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  const nlp::runtime::EmbeddingTable table{};
  std::cout << "position weights defined before reset = " << table.position_weights().defined() << "\n";
  return 0;
}
```

Output

```text
position weights defined before reset = 0
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
  std::cout << "restored position_weights is available\n";
  return 0;
}
```

Output

```text
restored position_weights is available
```

## See Also

- [`EmbeddingTable`](../EmbeddingTable.md) -- the table.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="pad_id.html">
    <span class="label">Previous</span>
    <span class="title">pad_id</span>
  </a>
  <a class="next-link" href="reset.html">
    <span class="label">Next</span>
    <span class="title">reset</span>
  </a>
</div>
<!-- apiref:nav:end -->
