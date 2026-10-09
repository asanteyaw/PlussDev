<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d4bfb03e917c -->
# `pluss::ai::nlp::runtime::LstmTextEncoder::model`

[Back to `pluss::ai::nlp::runtime::LstmTextEncoder`](../LstmTextEncoder.md)

## Signature

```cpp
pluss::nn::LSTM& pluss::ai::nlp::runtime::LstmTextEncoder::model() noexcept;
const pluss::nn::LSTM& pluss::ai::nlp::runtime::LstmTextEncoder::model() const noexcept;
```

## What It Does

Returns the underlying neural LSTM. Through it a caller reaches the LSTM's parameters, its training mode, and its
configuration, which is how the recurrence is trained, moved between devices, or saved. The wrapper handles the
packing; the LSTM does the recurrence.

## Parameters

None.

## Returns

A reference to the neural LSTM, mutable in the non-`const` overload and read-only in the `const` one.

## Errors

None. The method is `noexcept`.

## Practical Examples

### Example 1

Reading the hidden width and layer count from the underlying LSTM.

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
  std::cout << "hidden = " << encoder.model()->options().hidden_size << ", layers = " << encoder.model()->options().num_layers << "\n";
  return 0;
}
```

Output

```text
hidden = 3, layers = 1
```

### Example 2

Reading the input width the LSTM expects, which must match the embedding width.

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
  std::cout << "input_size = " << encoder.model()->options().input_size << ", embedding width = " << table.options().width << "\n";
  return 0;
}
```

Output

```text
input_size = 4, embedding width = 4
```

### Example 3

Checking the direction setting, which the LSTM fixed at construction.

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
  std::cout << "bidirectional = " << encoder.model()->options().bidirectional << "\n";
  return 0;
}
```

Output

```text
bidirectional = 0
```

## See Also

- [`forward`](forward.md) -- runs the encoder.
- [`LstmTextEncoder`](../LstmTextEncoder.md) -- the wrapper.

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
