<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c6a4100c16e5 -->
# `pluss::ai::nlp::runtime::TransformerTextEncoder::model`

[Back to `pluss::ai::nlp::runtime::TransformerTextEncoder`](../TransformerTextEncoder.md)

## Signature

```cpp
pluss::nn::TransformerEncoder& pluss::ai::nlp::runtime::TransformerTextEncoder::model() noexcept;
const pluss::nn::TransformerEncoder& pluss::ai::nlp::runtime::TransformerTextEncoder::model() const noexcept;
```

## What It Does

Returns the underlying neural transformer encoder. Through it a caller reaches the encoder's parameters, its
training mode, and its configuration, which is how the encoder is trained, moved between devices, or saved.
The wrapper adds the batch handling; the model does the attention.

## Parameters

None.

## Returns

A reference to the neural encoder, mutable in the non-`const` overload and read-only in the `const` one.

## Errors

None. The method is `noexcept`.

## Practical Examples

### Example 1

Reading the model dimension and layer count from the underlying encoder.

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
  std::cout << "d_model = " << encoder.model()->options().d_model << ", layers = " << encoder.model()->options().num_layers << "\n";
  return 0;
}
```

Output

```text
d_model = 4, layers = 1
```

### Example 2

Reading the head count, which the encoder fixed at construction.

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
  std::cout << "nhead = " << encoder.model()->options().nhead << "\n";
  return 0;
}
```

Output

```text
nhead = 2
```

### Example 3

Checking that the model's batch layout matches a batch-first batch.

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
  std::cout << "model batch_first = " << encoder.model()->options().batch_first << ", batch batch_first = " << batch.batch_first << "\n";
  return 0;
}
```

Output

```text
model batch_first = 1, batch batch_first = 1
```

## See Also

- [`forward`](forward.md) -- runs the encoder.
- [`TransformerTextEncoder`](../TransformerTextEncoder.md) -- the wrapper.

## Changelog

- **0.1** -- initial documented surface.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="forward.html">
    <span class="label">Previous</span>
    <span class="title">forward</span>
  </a>
</div>
<!-- apiref:nav:end -->
