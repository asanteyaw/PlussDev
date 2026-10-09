<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 44ad9fbd1af0 -->
# `pluss::ai::nlp::runtime::SequenceBatch::pack`

[Back to `pluss::ai::nlp::runtime::SequenceBatch`](../SequenceBatch.md)

## Signature

```cpp
pluss::nn::PackedSequence pluss::ai::nlp::runtime::SequenceBatch::pack(const pluss::Series& values) const;
```

## What It Does

Packs a padded, encoded batch into the packed form recurrent layers consume. The padding is removed:
each sequence keeps only its real elements, concatenated in the order the layer expects. A recurrent
layer run on the packed form never spends computation on padding and never lets padding affect the final
state of a short sequence.

The values must already have the batch's layout, `[batch, max_length, width]` for batch-first batches and
`[max_length, batch, width]` otherwise. The method checks this and fails when the shape does not match.

## Parameters

`values` is the encoded batch, typically the output of an embedding table, rank three, with the first two
dimensions matching the batch.

## Returns

A `pluss::nn::PackedSequence`, the input the recurrent layers accept. The lengths come from the batch's own
`lengths` series.

## Errors

- `values` is undefined, is not rank three, or its leading dimensions do not match the batch: the call
  fails with a shape error that names the expected layout.

## Practical Examples

Three examples: packing an embedded batch, the failure on a wrong shape, and the effect of packing on a
short sequence.

### Example 1

Packing the embedded batch of two documents of different lengths. The packed form carries the lengths, and
the program prints them.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  const auto docs = wf.infer(std::vector<std::string>{"Ada writes books.", "Kofi."});
  nlp::runtime::Indexer indexer{};
  indexer.learn(docs);
  const nlp::runtime::SequenceBatch batch = indexer.batch(docs).value();
  const nlp::runtime::EmbeddingTable table{nlp::runtime::EmbeddingOptions{.width = 4}};
  nlp::runtime::EmbeddingTable embedding{nlp::runtime::EmbeddingOptions{.width = 4}};
  embedding.reset(static_cast<std::int64_t>(indexer.vocabulary().size()), indexer.pad_id());
  const pluss::Series embedded = embedding.forward(batch);
  const pluss::nn::PackedSequence packed = batch.pack(embedded);
  std::cout << "packed batch built from shape " << batch.batch_size << " x " << batch.max_length << "\n";
  return 0;
}
```

Output

```text
packed batch built from shape 2 x 3
```

### Example 2

The shape check. A batch-first batch rejects values laid out as sequence-first, and the program prints the
first line of the error.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  const auto docs = wf.infer(std::vector<std::string>{"Ada writes.", "Kofi."});
  nlp::runtime::Indexer indexer{};
  indexer.learn(docs);
  const nlp::runtime::SequenceBatch batch = indexer.batch(docs).value();
  nlp::runtime::EmbeddingTable embedding{nlp::runtime::EmbeddingOptions{.width = 4}};
  embedding.reset(static_cast<std::int64_t>(indexer.vocabulary().size()), indexer.pad_id());
  const pluss::Series wrong = pluss::series::zeros({3, 2, 4}, pluss::Config{}.with_dtype(pluss::ScalarType::Float32));
  try {
    batch.pack(wrong);
    std::cout << "packed\n";
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

### Example 3

Why packing matters. A short sequence run through a recurrent layer on the packed form ends at its own last
element, not at the padded end of the batch, which the program shows by comparing the lengths.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  const auto docs = wf.infer(std::vector<std::string>{"Ada writes books.", "Kofi."});
  nlp::runtime::Indexer indexer{};
  indexer.learn(docs);
  const nlp::runtime::SequenceBatch batch = indexer.batch(docs).value();
  const auto lengths = batch.lengths.to(pluss::pCPU, pluss::ScalarType::Int64).contiguous().template to_vec<std::int64_t>();
  std::cout << "padded length = " << batch.max_length << "\n";
  for (std::size_t row = 0; row < lengths.size(); ++row) {
    std::cout << "sequence " << row << " has " << lengths[row] << " real element(s)\n";
  }
  return 0;
}
```

Output

```text
padded length = 3
sequence 0 has 3 real element(s)
sequence 1 has 1 real element(s)
```

## See Also

- [`SequenceBatch`](../SequenceBatch.md) -- the batch this method packs.
- [`LstmTextEncoder`](../LstmTextEncoder.md) -- consumes the packed form.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="empty.html">
    <span class="label">Previous</span>
    <span class="title">empty</span>
  </a>
</div>
<!-- apiref:nav:end -->
