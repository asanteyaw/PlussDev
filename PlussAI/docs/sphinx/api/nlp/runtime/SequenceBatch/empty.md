<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 615d7f83464a -->
# `pluss::ai::nlp::runtime::SequenceBatch::empty`

[Back to `pluss::ai::nlp::runtime::SequenceBatch`](../SequenceBatch.md)

## Signature

```cpp
bool pluss::ai::nlp::runtime::SequenceBatch::empty() const noexcept;
```

## What It Does

Reports whether the batch holds no sequence data: no rows, no columns, or no token ids. A batch the
indexer returns for a non-empty corpus is never empty, so the method mainly guards a default-constructed
batch before it reaches an encoder.

## Parameters

None.

## Returns

`true` when `batch_size` or `max_length` is zero, or when `token_ids` is undefined.

## Errors

None. The method is `noexcept`.

## Practical Examples

Three examples: a batch from the indexer, a default batch, and a batch from an empty document list.

### Example 1

A batch built from real documents is not empty, and the program prints its shape.

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
  std::cout << "empty = " << batch.empty() << ", shape = " << batch.batch_size << " x " << batch.max_length << "\n";
  return 0;
}
```

Output

```text
empty = 0, shape = 2 x 2
```

### Example 2

A default-constructed batch, which holds no data and reports itself empty.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  const nlp::runtime::SequenceBatch batch{};
  std::cout << "empty = " << batch.empty() << "\n";
  return 0;
}
```

Output

```text
empty = 1
```

### Example 3

A batch whose documents all contain no terms. The indexer refuses to build it, so the caller never holds
an empty batch from that path; the program reports the failure.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  nlp::runtime::Indexer indexer{};
  indexer.learn(wf.infer(std::vector<std::string>{"Ada."}));
  const auto result = indexer.batch(wf.infer(std::vector<std::string>{"..."}));
  std::cout << "ok = " << result.ok() << "\n";
  return 0;
}
```

Output

```text
ok = 0
```

## See Also

- [`SequenceBatch`](../SequenceBatch.md) -- the batch this method reports on.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="causal_mask.html">
    <span class="label">Previous</span>
    <span class="title">causal_mask</span>
  </a>
  <a class="next-link" href="pack.html">
    <span class="label">Next</span>
    <span class="title">pack</span>
  </a>
</div>
<!-- apiref:nav:end -->
