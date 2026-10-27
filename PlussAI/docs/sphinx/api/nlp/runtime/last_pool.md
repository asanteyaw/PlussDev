<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 000000000000 -->
# `pluss::ai::nlp::runtime::last_pool`

## Signature

```cpp
pluss::Series pluss::ai::nlp::runtime::last_pool(const pluss::Series& encoded, const pluss::ai::nlp::runtime::SequenceBatch& batch);
```

## What It Does

Takes each sequence's encoded vector at its last real position, index `lengths - 1`. Padding is skipped
by construction, since the position is read from the lengths rather than from the padded end. It suits
recurrent encoders, whose final state summarizes everything read so far.

The input must have the batch's layout, rank three, with the leading dimensions matching the batch.

## Parameters

`encoded` is the encoder output or the embedding output for the batch, rank three.

`batch` supplies the valid mask and the lengths.

## Returns

A `Series` of shape `[batch, width]`.

## Errors

- `encoded` does not match the batch layout: the call fails with a shape error that names the expected layout.
## Practical Examples

Three examples: the last vectors of two sentences, the last element compared with the mean, and a
sentence that ends in padding.

### Example 1

The last-element vector of two sentences of different lengths, printed as one row per sentence.

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
  const pluss::Series pooled = nlp::runtime::last_pool(embedding.forward(batch), batch);
  std::cout << "pooled shape = " << pooled.sizes()[0] << " x " << pooled.sizes()[1] << "\n";
  return 0;
}
```

Output

```text
pooled shape = 2 x 3
```

### Example 2

Last element against the mean. For a single-element sentence the two pools agree, which the program checks
against the mean pool of the same batch.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  const auto docs = wf.infer(std::vector<std::string>{"Kofi."});
  nlp::runtime::Indexer indexer{};
  indexer.learn(docs);
  const nlp::runtime::SequenceBatch batch = indexer.batch(docs).value();
  nlp::runtime::EmbeddingTable embedding{nlp::runtime::EmbeddingOptions{.width = 2}};
  embedding.reset(static_cast<std::int64_t>(indexer.vocabulary().size()), indexer.pad_id());
  const pluss::Series encoded = embedding.forward(batch);
  const auto last = nlp::runtime::last_pool(encoded, batch).to(pluss::pCPU, pluss::ScalarType::Float64).contiguous().template host<double>();
  const auto mean = nlp::runtime::mean_pool(encoded, batch).to(pluss::pCPU, pluss::ScalarType::Float64).contiguous().template host<double>();
  std::cout << "same for one element = " << (last == mean) << "\n";
  return 0;
}
```

Output

```text
same for one element = 1
```

### Example 3

A sentence followed by padding in a longer batch. The last vector is taken from the sentence's own last
element, not from the padded position at the end of the batch.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  const auto docs = wf.infer(std::vector<std::string>{"Ada writes good books now.", "Kofi."});
  nlp::runtime::Indexer indexer{};
  indexer.learn(docs);
  const nlp::runtime::SequenceBatch batch = indexer.batch(docs).value();
  const auto lengths = batch.lengths.to(pluss::pCPU, pluss::ScalarType::Int64).contiguous().template host<std::int64_t>();
  std::cout << "row 1 real length = " << lengths[1] << ", padded length = " << batch.max_length << "\n";
  return 0;
}
```

Output

```text
row 1 real length = 1, padded length = 5
```

## See Also

- [`mean_pool`](mean_pool.md) -- the average alternative.
- [`LstmTextEncoder`](LstmTextEncoder.md) -- its final state is the natural input to this pool.
