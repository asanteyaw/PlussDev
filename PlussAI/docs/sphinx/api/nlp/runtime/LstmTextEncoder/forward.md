<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: bec8dfa88597 -->
# `pluss::ai::nlp::runtime::LstmTextEncoder::forward`

[Back to `pluss::ai::nlp::runtime::LstmTextEncoder`](../LstmTextEncoder.md)

## Signature

```cpp
pluss::Series pluss::ai::nlp::runtime::LstmTextEncoder::forward(
    const pluss::Series& embedded,
    const pluss::ai::nlp::runtime::SequenceBatch& batch) const;
pluss::Series pluss::ai::nlp::runtime::LstmTextEncoder::forward(
    const pluss::ai::nlp::runtime::EmbeddingTable& table,
    const pluss::ai::nlp::runtime::SequenceBatch& batch) const;
```

## What It Does

Encodes a batch with a recurrent LSTM. The method checks the layout and the input width against the LSTM's
`input_size`, packs the batch so only real elements are processed, runs the recurrence, and unpacks the
result to the padded layout. Output at padded positions is zero. The overload that takes an embedding table
embeds the batch first.

## Parameters

`embedded` is the embedded batch, rank three, with the batch's leading dimensions.

`table` is the embedding table that produces the embedded batch from `batch`.

`batch` is the sequence batch the input was built from.

## Returns

A `Series` shaped `[batch, max_length, hidden_size]` for batch-first batches, or with the sequence first otherwise.

## Errors

- The embedded input does not match the batch layout: the call fails with a shape error.
- The LSTM's `batch_first` does not match the batch's: the call fails with a precondition error.
- The embedded width differs from `input_size`: the call fails with a precondition error.

## Practical Examples

### Example 1

Encoding a batch through the table. The output shape is printed as batch, sequence, and hidden width.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  nlp::runtime::Indexer indexer{nlp::runtime::SequenceOptions{.batch_first = true}};
  indexer.learn(wf.infer(std::vector<std::string>{"Ada writes books.", "Kofi reads."}));
  const nlp::runtime::SequenceBatch batch = indexer.batch(wf.infer(std::vector<std::string>{"Ada reads.", "Kofi."})).value();
  nlp::runtime::EmbeddingTable table{nlp::runtime::EmbeddingOptions{.width = 4}};
  table.reset(static_cast<std::int64_t>(indexer.vocabulary().size()), indexer.pad_id());
  nlp::runtime::LstmTextEncoder encoder{pluss::nn::LSTMOptions{.input_size = 4, .hidden_size = 3, .batch_first = true}};
  const pluss::Series out = encoder.forward(table, batch);
  std::cout << "shape = " << out.sizes()[0] << " x " << out.sizes()[1] << " x " << out.sizes()[2] << "\n";
  return 0;
}
```

Output

```text
shape = 2 x 2 x 3
```

### Example 2

The padded positions are zero. Row one is the shorter sentence, so its last position is padding, and the program prints that position's first value.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  nlp::runtime::Indexer indexer{nlp::runtime::SequenceOptions{.batch_first = true}};
  indexer.learn(wf.infer(std::vector<std::string>{"Ada writes books.", "Kofi reads."}));
  const nlp::runtime::SequenceBatch batch = indexer.batch(wf.infer(std::vector<std::string>{"Ada reads.", "Kofi."})).value();
  nlp::runtime::EmbeddingTable table{nlp::runtime::EmbeddingOptions{.width = 4}};
  table.reset(static_cast<std::int64_t>(indexer.vocabulary().size()), indexer.pad_id());
  nlp::runtime::LstmTextEncoder encoder{pluss::nn::LSTMOptions{.input_size = 4, .hidden_size = 3, .batch_first = true}};
  const auto values = encoder.forward(table.forward(batch), batch).to(pluss::pCPU, pluss::ScalarType::Float64).contiguous().template to_vec<double>();
  const std::size_t padded = static_cast<std::size_t>((1 * batch.max_length + 1) * 3);
  std::cout << "padded output first value = " << values[padded] << "\n";
  return 0;
}
```

Output

```text
padded output first value = 0
```

### Example 3

The input width mismatch. The LSTM expects an input width of five, but the table produces four, so the call is refused; the program prints the first line of the message.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  nlp::runtime::Indexer indexer{nlp::runtime::SequenceOptions{.batch_first = true}};
  indexer.learn(wf.infer(std::vector<std::string>{"Ada writes books.", "Kofi reads."}));
  const nlp::runtime::SequenceBatch batch = indexer.batch(wf.infer(std::vector<std::string>{"Ada reads.", "Kofi."})).value();
  nlp::runtime::EmbeddingTable table{nlp::runtime::EmbeddingOptions{.width = 4}};
  table.reset(static_cast<std::int64_t>(indexer.vocabulary().size()), indexer.pad_id());
  nlp::runtime::LstmTextEncoder encoder{pluss::nn::LSTMOptions{.input_size = 5, .hidden_size = 3, .batch_first = true}};
  try {
    encoder.forward(table, batch);
    std::cout << "encoded\n";
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

- [`model`](../LstmTextEncoder/model.md) -- the underlying LSTM.
- [`LstmTextEncoder`](../LstmTextEncoder.md) -- the encoder.

## Changelog

- **0.1** -- initial documented surface.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="model.html">
    <span class="label">Next</span>
    <span class="title">model</span>
  </a>
</div>
<!-- apiref:nav:end -->
