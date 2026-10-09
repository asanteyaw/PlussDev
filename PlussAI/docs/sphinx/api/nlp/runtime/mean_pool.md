<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 000000000000 -->
# `pluss::ai::nlp::runtime::mean_pool`

## Signature

```cpp
pluss::Series pluss::ai::nlp::runtime::mean_pool(const pluss::Series& encoded, const pluss::ai::nlp::runtime::SequenceBatch& batch);
```

## What It Does

Averages each sequence's encoded elements over its real positions. Padding is excluded: the sum runs only
over the valid entries, and the divisor is the sequence's length, clamped to at least one so an empty
sequence yields zeros rather than a division by zero. The result is one vector per sequence, which is the
usual summary of a sentence for a classifier.

The input must have the batch's layout, rank three, with the leading dimensions matching the batch.

## Parameters

`encoded` is the encoder output or the embedding output for the batch, rank three.

`batch` supplies the valid mask and the lengths.

## Returns

A `Series` of shape `[batch, width]`.

## Errors

- `encoded` does not match the batch layout: the call fails with a shape error that names the expected layout.

## Practical Examples

Three examples: the mean vector of two sentences, the effect of padding on the mean of a short sentence,
and the empty-sequence case.

### Example 1

The mean-pooled vector of two sentences, printed as one row per sentence.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  const auto docs = wf.infer(std::vector<std::string>{"Ada writes books.", "Kofi reads."});
  nlp::runtime::Indexer indexer{};
  indexer.learn(docs);
  const nlp::runtime::SequenceBatch batch = indexer.batch(docs).value();
  nlp::runtime::EmbeddingTable embedding{nlp::runtime::EmbeddingOptions{.width = 3}};
  embedding.reset(static_cast<std::int64_t>(indexer.vocabulary().size()), indexer.pad_id());
  const pluss::Series pooled = nlp::runtime::mean_pool(embedding.forward(batch), batch);
  std::cout << "pooled shape = " << pooled.sizes()[0] << " x " << pooled.sizes()[1] << "\n";
  return 0;
}
```

Output

```text
pooled shape = 2 x 3
```

### Example 2

Padding does not dilute the mean. The short sentence is pooled once in a batch with a longer one and once
alone; the program prints both results, which agree.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  const auto short_doc = wf.infer(std::vector<std::string>{"Kofi."});
  const auto long_doc = wf.infer(std::vector<std::string>{"Ada writes good books now.", "Kofi."});
  nlp::runtime::Indexer indexer{};
  indexer.learn(long_doc);
  nlp::runtime::EmbeddingTable embedding{nlp::runtime::EmbeddingOptions{.width = 2}};
  embedding.reset(static_cast<std::int64_t>(indexer.vocabulary().size()), indexer.pad_id());
  const auto alone = nlp::runtime::mean_pool(embedding.forward(indexer.batch(short_doc).value()), indexer.batch(short_doc).value())
                         .to(pluss::pCPU, pluss::ScalarType::Float64).contiguous().template to_vec<double>();
  const auto together = nlp::runtime::mean_pool(embedding.forward(indexer.batch(long_doc).value()), indexer.batch(long_doc).value())
                            .to(pluss::pCPU, pluss::ScalarType::Float64).contiguous().template to_vec<double>();
  std::cout << "alone:    " << alone[0] << " " << alone[1] << "\n";
  std::cout << "in batch: " << together[2] << " " << together[3] << "\n";
  return 0;
}
```

Output

```text
alone:    -0.0187743 0.107911
in batch: -0.0187743 0.107911
```

### Example 3

The empty-sequence case. A row with no real elements pools to zeros, because the divisor is clamped, and the
program prints the pooled row.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  const auto docs = wf.infer(std::vector<std::string>{"Ada writes.", "..."});
  nlp::runtime::Indexer indexer{};
  indexer.learn(docs);
  const nlp::runtime::SequenceBatch batch = indexer.batch(docs).value();
  nlp::runtime::EmbeddingTable embedding{nlp::runtime::EmbeddingOptions{.width = 2}};
  embedding.reset(static_cast<std::int64_t>(indexer.vocabulary().size()), indexer.pad_id());
  const auto pooled = nlp::runtime::mean_pool(embedding.forward(batch), batch)
                          .to(pluss::pCPU, pluss::ScalarType::Float64).contiguous().template to_vec<double>();
  std::cout << "empty row pooled = " << pooled[2] << " " << pooled[3] << "\n";
  return 0;
}
```

Output

```text
empty row pooled = 0 0
```

## See Also

- [`last_pool`](last_pool.md) -- the last-element alternative.
- [`SequenceBatch`](SequenceBatch.md) -- supplies the lengths and masks.
