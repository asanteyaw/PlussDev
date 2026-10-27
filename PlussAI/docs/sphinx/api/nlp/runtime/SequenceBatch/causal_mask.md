<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6fe2967e231b -->
# `pluss::ai::nlp::runtime::SequenceBatch::causal_mask`

[Back to `pluss::ai::nlp::runtime::SequenceBatch`](../SequenceBatch.md)

## Signature

```cpp
pluss::Series pluss::ai::nlp::runtime::SequenceBatch::causal_mask() const;
```

## What It Does

Builds the square mask that stops each position from attending to the positions after it. It is shaped
`[max_length, max_length]`, and the entries above the diagonal are blocked. A causal encoder run with this
mask reads each sequence left to right, so the representation at a position depends only on what came
before it.

The mask is the same for every row of the batch, because it depends only on the longest length. Padding is
handled separately by the key-padding mask the batch already carries.

## Parameters

None.

## Returns

A `Series` mask of shape `[max_length, max_length]`, usable as the attention mask of a causal encoder.

## Errors

- The batch's `max_length` is zero: the method fails with a precondition error.

## Practical Examples

Three examples: the mask of a batch, the mask's shape for a longer sequence, and the failure on an empty
batch.

### Example 1

The causal mask of a batch whose longest sequence has three elements, printed row by row so the blocked
entries are visible.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  const auto docs = wf.infer(std::vector<std::string>{"Ada writes books."});
  nlp::runtime::Indexer indexer{};
  indexer.learn(docs);
  const nlp::runtime::SequenceBatch batch = indexer.batch(docs).value();
  const auto mask = batch.causal_mask().to(pluss::pCPU, pluss::ScalarType::Float64).contiguous().template host<double>();
  const std::int64_t n = batch.max_length;
  for (std::int64_t row = 0; row < n; ++row) {
    std::cout << "row " << row << ":";
    for (std::int64_t col = 0; col < n; ++col) std::cout << " " << mask[static_cast<std::size_t>(row * n + col)];
    std::cout << "\n";
  }
  return 0;
}
```

Output

```text
row 0: 0 1 1
row 1: 0 0 1
row 2: 0 0 0
```

### Example 2

The mask's size follows the longest sequence. A batch with a single five-element sentence gives a five by
five mask, and the program prints its shape.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  const auto docs = wf.infer(std::vector<std::string>{"Ada writes good books now."});
  nlp::runtime::Indexer indexer{};
  indexer.learn(docs);
  const nlp::runtime::SequenceBatch batch = indexer.batch(docs).value();
  const pluss::Series mask = batch.causal_mask();
  std::cout << "mask shape = " << mask.sizes()[0] << " x " << mask.sizes()[1] << "\n";
  return 0;
}
```

Output

```text
mask shape = 5 x 5
```

### Example 3

The failure on a batch that was never built. A default batch has no length, so the mask cannot be formed,
and the program prints the first line of the error.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>

int main() {
  namespace nlp = pluss::ai::nlp;

  const nlp::runtime::SequenceBatch batch{};
  try {
    batch.causal_mask();
    std::cout << "mask built\n";
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

- [`SequenceBatch`](../SequenceBatch.md) -- the batch this method masks.
- [`TransformerTextEncoder`](../TransformerTextEncoder.md) -- consumes the causal mask.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="empty.html">
    <span class="label">Next</span>
    <span class="title">empty</span>
  </a>
</div>
<!-- apiref:nav:end -->
