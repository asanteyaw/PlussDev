<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: cdd4b00b9298 -->
# `pluss::ai::nlp::runtime::Indexer::dump_state_lines`

[Back to `pluss::ai::nlp::runtime::Indexer`](../Indexer.md)

## Signature

```cpp
std::vector<std::string> pluss::ai::nlp::runtime::Indexer::dump_state_lines() const;
```

## What It Does

Writes the indexer's state as a list of `key=value` lines: the options, the four reserved ids, the
vocabulary count, and one line per vocabulary entry. Text values are hex-encoded, so any token survives
the trip through a text file. [`load_state_lines`](load_state_lines.md) reads the same lines back.

## Parameters

None.

## Returns

A `std::vector<std::string>`, one line per entry. The number of lines is the vocabulary size plus a fixed
header of options and ids.

## Errors

None. The method is `const` and does not fail; an unlearned indexer writes its options and an empty
vocabulary.

## Practical Examples

Three examples: the lines of a small learned indexer, the header of an unlearned one, and a saved indexer's
line count.

### Example 1

The lines of a learned indexer, printed in full so the format is visible.

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
  for (const std::string& line : indexer.dump_state_lines()) std::cout << line << "\n";
  return 0;
}
```

Output

```text
use_lemmas=1
skip_punct=1
lowercase=1
add_bos=0
add_eos=0
batch_first=1
pad_token=3c7061643e
unk_token=3c756e6b3e
bos_token=3c626f733e
eos_token=3c656f733e
pad_id=0
unk_id=1
bos_id=-1
eos_id=-1
vocab_count=3
vocab_0=3c7061643e
vocab_1=3c756e6b3e
vocab_2=616461
```

### Example 2

The header of an unlearned indexer. It has options and reserved ids set to `-1`, and an empty vocabulary.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  const nlp::runtime::Indexer indexer{};
  for (const std::string& line : indexer.dump_state_lines()) {
    if (line.rfind("vocab_", 0) != 0) std::cout << line << "\n";
  }
  return 0;
}
```

Output

```text
use_lemmas=1
skip_punct=1
lowercase=1
add_bos=0
add_eos=0
batch_first=1
pad_token=3c7061643e
unk_token=3c756e6b3e
bos_token=3c626f733e
eos_token=3c656f733e
pad_id=-1
unk_id=-1
bos_id=-1
eos_id=-1
```

### Example 3

The number of lines grows with the vocabulary. The program saves two indexers that differ only in corpus size
and prints each line count.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  nlp::runtime::Indexer small{};
  small.learn(wf.infer(std::vector<std::string>{"Ada."}));
  nlp::runtime::Indexer large{};
  large.learn(wf.infer(std::vector<std::string>{"Ada writes books.", "Kofi reads notes."}));
  std::cout << "small lines = " << small.dump_state_lines().size() << "\n";
  std::cout << "large lines = " << large.dump_state_lines().size() << "\n";
  return 0;
}
```

Output

```text
small lines = 18
large lines = 23
```

## See Also

- [`load_state_lines`](load_state_lines.md) -- restores these lines.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="bos_id.html">
    <span class="label">Previous</span>
    <span class="title">bos_id</span>
  </a>
  <a class="next-link" href="eos_id.html">
    <span class="label">Next</span>
    <span class="title">eos_id</span>
  </a>
</div>
<!-- apiref:nav:end -->
