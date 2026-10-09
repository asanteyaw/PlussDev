<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 070d02f2f164 -->
# `pluss::ai::nlp::runtime::Indexer::bos_id`

[Back to `pluss::ai::nlp::runtime::Indexer`](../Indexer.md)

## Signature

```cpp
std::int64_t pluss::ai::nlp::runtime::Indexer::bos_id() const noexcept;
```

## What It Does

Returns the id of the bos token. The id of the beginning-of-sequence token. It exists only when `add_bos` is on; otherwise it is `-1`.

## Parameters

None.

## Returns

A `std::int64_t`. `-1` when `add_bos` is off or the indexer has not learned.

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
  std::cout << "bos_id = " << indexer.bos_id() << "\n";
  return 0;
}
```

Output

```text
bos_id = -1
```

### Example 2

The id on a new indexer, before any learning.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  const nlp::runtime::Indexer indexer{};
  std::cout << "bos_id before learn = " << indexer.bos_id() << "\n";
  return 0;
}
```

Output

```text
bos_id before learn = -1
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
  <a class="prev-link" href="batch.html">
    <span class="label">Previous</span>
    <span class="title">batch</span>
  </a>
  <a class="next-link" href="dump_state_lines.html">
    <span class="label">Next</span>
    <span class="title">dump_state_lines</span>
  </a>
</div>
<!-- apiref:nav:end -->
