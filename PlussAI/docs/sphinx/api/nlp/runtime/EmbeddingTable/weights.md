<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e2b01f217aca -->
# `pluss::ai::nlp::runtime::EmbeddingTable::weights`

[Back to `pluss::ai::nlp::runtime::EmbeddingTable`](../EmbeddingTable.md)

## Signature

```cpp
const pluss::Series& pluss::ai::nlp::runtime::EmbeddingTable::weights() const noexcept;
```

## What It Does

Returns the token weight matrix, shaped `[vocab size, width]`. Its row `i` is the vector for vocabulary id `i`, and the pad row is zero.

## Parameters

None.

## Returns

A `const` reference to the weights. It is undefined until the table is reset or loaded.

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
  std::cout << "weights shape = " << table.weights().sizes()[0] << " x " << table.weights().sizes()[1] << "\n";
  return 0;
}
```

Output

```text
weights shape = 7 x 4
```

### Example 2

The value on a new table, before reset.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  const nlp::runtime::EmbeddingTable table{};
  std::cout << "weights defined before reset = " << table.weights().defined() << "\n";
  return 0;
}
```

Output

```text
weights defined before reset = 0
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
  std::cout << "restored weights is available\n";
  return 0;
}
```

Output

```text
restored weights is available
```

## See Also

- [`EmbeddingTable`](../EmbeddingTable.md) -- the table.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="vocab_size.html">
    <span class="label">Previous</span>
    <span class="title">vocab_size</span>
  </a>
</div>
<!-- apiref:nav:end -->
