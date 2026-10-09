<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 5a96317d0920 -->
# `pluss::ai::nlp::runtime::Indexer::pad_id`

[Back to `pluss::ai::nlp::runtime::Indexer`](../Indexer.md)

## Signature

```cpp
std::int64_t pluss::ai::nlp::runtime::Indexer::pad_id() const noexcept;
```

## What It Does

Returns the id of the pad token. The id of the pad token. It is always `0` for a learned indexer, and it is the value a batch uses to fill short rows. Embedding tables zero the row for this id so padding carries no weight.

## Parameters

None.

## Returns

A `std::int64_t`. `-1` only when the indexer has not learned a vocabulary.

## Errors

None. The method is `noexcept`.

## Practical Examples

Three examples: the id on a learned indexer, the id on a new indexer, and the id beside the other reserved
ids.

### Example 1

The id on a learned indexer with the default options.

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
  std::cout << "pad_id = " << indexer.pad_id() << "\n";
  return 0;
}
```

Output

```text
pad_id = 0
```

### Example 2

The id on a new indexer, before any learning.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  const nlp::runtime::Indexer indexer{};
  std::cout << "pad_id before learn = " << indexer.pad_id() << "\n";
  return 0;
}
```

Output

```text
pad_id before learn = -1
```

### Example 3

The reserved ids side by side, with the boundary tokens enabled so every one of them has a value.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  nlp::runtime::Indexer indexer{nlp::runtime::SequenceOptions{.add_bos = true, .add_eos = true}};
  indexer.learn(wf.infer(std::vector<std::string>{"Ada writes."}));
  std::cout << "pad = " << indexer.pad_id() << ", unk = " << indexer.unk_id()
            << ", bos = " << indexer.bos_id() << ", eos = " << indexer.eos_id() << "\n";
  return 0;
}
```

Output

```text
pad = 0, unk = 1, bos = 2, eos = 3
```

## See Also

- [`vocabulary`](vocabulary.md) -- the full list these ids index.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="options.html">
    <span class="label">Previous</span>
    <span class="title">options</span>
  </a>
  <a class="next-link" href="unk_id.html">
    <span class="label">Next</span>
    <span class="title">unk_id</span>
  </a>
</div>
<!-- apiref:nav:end -->
