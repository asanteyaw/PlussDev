<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: dfe5fe93f3e3 -->
# `pluss::ai::nlp::runtime::EmbeddingTable::reset`

[Back to `pluss::ai::nlp::runtime::EmbeddingTable`](../EmbeddingTable.md)

## Signature

```cpp
void pluss::ai::nlp::runtime::EmbeddingTable::reset(std::int64_t vocab_size, std::int64_t pad_id = -1);
```

## What It Does

Allocates the table's weights for a vocabulary of `vocab_size` ids and `width` columns. The token weights are
drawn uniformly from `[-s, s]` with `s = 1 / sqrt(width)`, seeded by the options. If `pad_id` names a valid
row, that row is zeroed. Position weights are allocated for `max_positions` positions: learned ones drawn the
same way, or sinusoidal ones when `learned_positions` is off.

Resetting replaces any existing weights. The table must be reset to a vocabulary the indexer produced,
because a mismatched size would look up ids that are out of range.

## Parameters

`vocab_size` is the number of ids the table must hold, normally `indexer.vocabulary().size()`.

`pad_id` is the id whose row is zeroed, normally `indexer.pad_id()`. A negative value, or one outside the
vocabulary, leaves every row random.

## Returns

Nothing.

## Errors

- `vocab_size` is not positive, or `width` is not positive: the call fails with a precondition error.
- Positions are enabled and `max_positions` is not positive: the call fails with a precondition error.

## Practical Examples

Three examples: resetting for an indexer's vocabulary, the zeroed pad row, and the rejected zero-size vocabulary.

### Example 1

Resetting a table for an indexer's vocabulary. The program prints the weight matrix's shape.

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

The pad row is zero. The program reads the row for the pad id and prints its sum, which is exactly zero.

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
  const auto weights = table.weights().to(pluss::pCPU, pluss::ScalarType::Float64).contiguous().template to_vec<double>();
  double pad_sum = 0.0;
  for (std::int64_t col = 0; col < 4; ++col) pad_sum += weights[static_cast<std::size_t>(indexer.pad_id() * 4 + col)];
  std::cout << "pad row sum = " << pad_sum << "\n";
  return 0;
}
```

Output

```text
pad row sum = 0
```

### Example 3

The rejected size. A vocabulary of zero is refused, and the program prints the first line of the message.

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
  try {
    table.reset(0);
    std::cout << "reset\n";
  } catch (const std::exception& error) {
    std::cout << "rejected\n";
  }
  return 0;
}
```

Output

```text
rejected
```

## See Also

- [`forward`](forward.md) -- uses the weights this method allocates.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="position_weights.html">
    <span class="label">Previous</span>
    <span class="title">position_weights</span>
  </a>
  <a class="next-link" href="vocab_size.html">
    <span class="label">Next</span>
    <span class="title">vocab_size</span>
  </a>
</div>
<!-- apiref:nav:end -->
