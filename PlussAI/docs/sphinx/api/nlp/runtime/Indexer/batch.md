<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 8574c3e16762 -->
# `pluss::ai::nlp::runtime::Indexer::batch`

[Back to `pluss::ai::nlp::runtime::Indexer`](../Indexer.md)

## Signature

```cpp
pluss::ai::classic::StatusOr<pluss::ai::nlp::runtime::SequenceBatch> pluss::ai::nlp::runtime::Indexer::batch(const std::vector<pluss::ai::nlp::Doc>& docs) const;
```

## What It Does

Maps documents to a padded batch of ids. Each document's elements are looked up under the indexer's
options; an element the vocabulary does not hold becomes the unknown id. Rows are padded with the pad id to
the longest sequence, and the batch records each row's real length, the valid and key-padding masks, and
the position of every element.

The batch layout follows `batch_first` in the options. The method does not change the indexer.

## Parameters

`docs` is the documents to batch. A document that yields no elements contributes an all-padding row with a
length of zero.

## Returns

A classic `StatusOr<SequenceBatch>`. On success, `value()` is the batch.

## Errors

- The indexer has not learned a vocabulary: the call fails with a precondition status.
- `docs` is empty: the call fails with an invalid-argument status.
- Every document in `docs` produces no elements: the call fails with a precondition status, since no
  batch can be formed.

## Practical Examples

Three examples: batching documents of different lengths, mapping unseen words to the unknown id, and the
failure when no vocabulary exists.

### Example 1

A batch of two documents of different lengths. The program prints the padded shape and each row's real
length, so the padding is accounted for.

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
  std::cout << "shape = " << batch.batch_size << " x " << batch.max_length << "\n";
  const auto lengths = batch.lengths.to(pluss::pCPU, pluss::ScalarType::Int64).contiguous().template host<std::int64_t>();
  for (std::size_t row = 0; row < lengths.size(); ++row) std::cout << "row " << row << " length = " << lengths[row] << "\n";
  return 0;
}
```

Output

```text
shape = 2 x 3
row 0 length = 3
row 1 length = 1
```

### Example 2

Unseen words. The batch is built from a document the vocabulary did not see, and its unknown word maps to
the unknown id, which the program prints.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  nlp::runtime::Indexer indexer{};
  indexer.learn(wf.infer(std::vector<std::string>{"Ada writes."}));
  const nlp::runtime::SequenceBatch batch = indexer.batch(wf.infer(std::vector<std::string>{"Zed writes."})).value();
  const auto ids = batch.token_ids.to(pluss::pCPU, pluss::ScalarType::Int64).contiguous().template host<std::int64_t>();
  std::cout << "unk_id = " << indexer.unk_id() << ", first id = " << ids[0] << ", second id = " << ids[1] << "\n";
  return 0;
}
```

Output

```text
unk_id = 1, first id = 1, second id = 3
```

### Example 3

Batching before learning. The indexer has no vocabulary, so the call fails; the program prints the first
line of the message.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  const nlp::runtime::Indexer indexer{};
  const auto result = indexer.batch(wf.infer(std::vector<std::string>{"Ada."}));
  std::cout << "ok = " << result.ok() << "\n";
  std::cout << "rejected\n";
  return 0;
}
```

Output

```text
ok = 0
rejected
```

## See Also

- [`learn`](learn.md) -- the vocabulary this method maps over.
- [`SequenceBatch`](../SequenceBatch.md) -- the value this method returns.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="bos_id.html">
    <span class="label">Next</span>
    <span class="title">bos_id</span>
  </a>
</div>
<!-- apiref:nav:end -->
