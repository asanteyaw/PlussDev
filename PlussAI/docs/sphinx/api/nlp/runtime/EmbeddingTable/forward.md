<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: b4b2d050efb6 -->
# `pluss::ai::nlp::runtime::EmbeddingTable::forward`

[Back to `pluss::ai::nlp::runtime::EmbeddingTable`](../EmbeddingTable.md)

## Signature

```cpp
pluss::Series pluss::ai::nlp::runtime::EmbeddingTable::forward(const pluss::ai::nlp::runtime::SequenceBatch& batch) const;
```

## What It Does

Looks up the vector of every id in the batch, adds the position vector of every position when positions are
used, and zeroes the entries that are padding. The output has the batch's layout with `width` as its last
dimension: `[batch, max_length, width]` for batch-first batches and `[max_length, batch, width]` otherwise.

The method does not change the table. Its output is what an encoder or a pooling step reads.

## Parameters

`batch` is a batch the indexer produced for the same vocabulary the table was reset with.

## Returns

A `Series` with the batch's layout and the table's width.

## Errors

- The table has not been reset or loaded: the call fails with a precondition error.
- The batch is empty: the call fails with a precondition error.
- Positions are used and the batch is longer than `max_positions`: the call fails with a precondition error.

## Practical Examples

Three examples: the shape of an embedded batch, the zero at a padded position, and the failure on an
unreset table.

### Example 1

The embedded shape of the batch, printed as batch, sequence, and width.

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
  const pluss::Series out = table.forward(batch);
  std::cout << "shape = " << out.sizes()[0] << " x " << out.sizes()[1] << " x " << out.sizes()[2] << "\n";
  return 0;
}
```

Output

```text
shape = 2 x 2 x 4
```

### Example 2

Padding gives zero. The second row of the batch is shorter than the first, so its last position is padding,
and the program prints that position's values, which are all zero.

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
  const auto values = table.forward(batch).to(pluss::pCPU, pluss::ScalarType::Float64).contiguous().template to_vec<double>();
  const std::size_t pad_position = static_cast<std::size_t>(1 * batch.max_length * 4 + 1 * 4);
  std::cout << "padded position = " << values[pad_position] << " " << values[pad_position + 1] << " "
            << values[pad_position + 2] << " " << values[pad_position + 3] << "\n";
  return 0;
}
```

Output

```text
padded position = -0 0 0 0
```

### Example 3

The unreset table. A table that was never reset has no weights, so the lookup fails; the program prints the
first line of the message.

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
  const nlp::runtime::EmbeddingTable table{nlp::runtime::EmbeddingOptions{.width = 4}};
  try {
    table.forward(batch);
    std::cout << "forward\n";
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

- [`reset`](reset.md) -- allocates the weights this method reads.
- [`TransformerTextEncoder`](../TransformerTextEncoder.md) -- consumes the output.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dump_state_lines.html">
    <span class="label">Previous</span>
    <span class="title">dump_state_lines</span>
  </a>
  <a class="next-link" href="load_state_lines.html">
    <span class="label">Next</span>
    <span class="title">load_state_lines</span>
  </a>
</div>
<!-- apiref:nav:end -->
