<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 167351ad2034 -->
# `pluss::ai::nlp::runtime::TransformerTextEncoder::forward`

[Back to `pluss::ai::nlp::runtime::TransformerTextEncoder`](../TransformerTextEncoder.md)

## Signature

```cpp
pluss::Series pluss::ai::nlp::runtime::TransformerTextEncoder::forward(
    const pluss::Series& embedded,
    const pluss::ai::nlp::runtime::SequenceBatch& batch,
    bool causal = false) const;
pluss::Series pluss::ai::nlp::runtime::TransformerTextEncoder::forward(
    const pluss::ai::nlp::runtime::EmbeddingTable& table,
    const pluss::ai::nlp::runtime::SequenceBatch& batch,
    bool causal = false) const;
```

## What It Does

Encodes a batch with self-attention. The method checks that the embedded input has the batch's layout and
that its width equals the model's `d_model`, then builds the key-padding mask from the batch so padding is
ignored. When `causal` is true it also applies the batch's causal mask, so each position sees only the
positions before it. The overload that takes an embedding table embeds the batch first and then encodes it.

## Parameters

`embedded` is the embedded batch, rank three, with the batch's leading dimensions.

`table` is the embedding table that produces the embedded batch from `batch`.

`batch` is the sequence batch the input was built from.

`causal` restricts each position to earlier positions when true. It defaults to false.

## Returns

A `Series` with the batch's layout and the model dimension as its last dimension.

## Errors

- The embedded input does not match the batch layout: the call fails with a shape error.
- The encoder's `batch_first` does not match the batch's: the call fails with a precondition error.
- The embedded width differs from `d_model`: the call fails with a precondition error.

## Practical Examples

### Example 1

Encoding a batch through the table. The output shape is printed as batch, sequence, and model width.

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
  nlp::runtime::TransformerTextEncoder encoder{pluss::nn::TransformerEncoderOptions{.d_model = 4, .nhead = 2, .num_layers = 1, .dim_feedforward = 8, .dropout = 0.0, .batch_first = true}};
  const pluss::Series out = encoder.forward(table, batch);
  std::cout << "shape = " << out.sizes()[0] << " x " << out.sizes()[1] << " x " << out.sizes()[2] << "\n";
  return 0;
}
```

Output

```text
shape = 2 x 2 x 4
```

### Example 2

Encoding an already embedded batch with the causal mask. The program embeds the batch itself and passes the result in.

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
  nlp::runtime::TransformerTextEncoder encoder{pluss::nn::TransformerEncoderOptions{.d_model = 4, .nhead = 2, .num_layers = 1, .dim_feedforward = 8, .dropout = 0.0, .batch_first = true}};
  const pluss::Series embedded = table.forward(batch);
  const pluss::Series out = encoder.forward(embedded, batch, true);
  std::cout << "causal shape = " << out.sizes()[0] << " x " << out.sizes()[1] << " x " << out.sizes()[2] << "\n";
  return 0;
}
```

Output

```text
causal shape = 2 x 2 x 4
```

### Example 3

The width mismatch. The encoder's model width is six, but the table produces four, so the call is refused; the program prints the first line of the message.

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
  nlp::runtime::TransformerTextEncoder encoder{pluss::nn::TransformerEncoderOptions{.d_model = 6, .nhead = 2, .num_layers = 1, .dim_feedforward = 8, .dropout = 0.0, .batch_first = true}};
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

- [`model`](../TransformerTextEncoder/model.md) -- the underlying neural encoder.
- [`TransformerTextEncoder`](../TransformerTextEncoder.md) -- the encoder.

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
