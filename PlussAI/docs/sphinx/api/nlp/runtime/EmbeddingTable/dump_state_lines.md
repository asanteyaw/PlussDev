<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 2578aede9ed3 -->
# `pluss::ai::nlp::runtime::EmbeddingTable::dump_state_lines`

[Back to `pluss::ai::nlp::runtime::EmbeddingTable`](../EmbeddingTable.md)

## Signature

```cpp
std::vector<std::string> pluss::ai::nlp::runtime::EmbeddingTable::dump_state_lines() const;
```

## What It Does

Writes the table's state as `key=value` lines: the width, the position policy, the seed, the vocabulary size,
the pad id, and both weight matrices as shapes and numbers. [`load_state_lines`](load_state_lines.md) reads the
same lines back, so a saved table reproduces the vectors of the original.

## Parameters

None.

## Returns

A `std::vector<std::string>`, one line per entry. The matrices are written as comma-separated numbers on single
lines, so the line count does not grow with the vocabulary.

## Errors

None. The method is `const` and does not fail. A table that was never reset writes empty matrices.

## Practical Examples

Three examples: the header lines of a reset table, the length of the matrix line, and the lines of an unreset
table.

### Example 1

The header of a reset table, printed with the key names.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  nlp::runtime::Indexer indexer{};
  indexer.learn(wf.infer(std::vector<std::string>{"Ada writes books.", "Kofi reads."}));
  const nlp::runtime::SequenceBatch batch = indexer.batch(wf.infer(std::vector<std::string>{"Ada reads.", "Kofi."})).value();
  nlp::runtime::EmbeddingTable table{nlp::runtime::EmbeddingOptions{.width = 4}};
  table.reset(static_cast<std::int64_t>(indexer.vocabulary().size()), indexer.pad_id());
  for (const std::string& line : table.dump_state_lines()) {
    if (line.rfind("weights=", 0) != 0 && line.rfind("positions=", 0) != 0) std::cout << line << "\n";
  }
  return 0;
}
```

Output

```text
width=4
use_positions=1
learned_positions=1
max_positions=4096
seed=0
vocab_size=7
pad_id=0
weight_rows=7
weight_cols=4
pos_rows=4096
pos_cols=4
```

### Example 2

The matrix line. Its length is the number of weights, which the program counts from the saved line.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  nlp::runtime::Indexer indexer{};
  indexer.learn(wf.infer(std::vector<std::string>{"Ada writes books.", "Kofi reads."}));
  const nlp::runtime::SequenceBatch batch = indexer.batch(wf.infer(std::vector<std::string>{"Ada reads.", "Kofi."})).value();
  nlp::runtime::EmbeddingTable table{nlp::runtime::EmbeddingOptions{.width = 4}};
  table.reset(static_cast<std::int64_t>(indexer.vocabulary().size()), indexer.pad_id());
  for (const std::string& line : table.dump_state_lines()) {
    if (line.rfind("weights=", 0) == 0) {
      std::cout << "weight values = " << (std::count(line.begin(), line.end(), ',') + 1) << "\n";
    }
  }
  return 0;
}
```

Output

```text
weight values = 1
```

### Example 3

An unreset table writes its options and empty matrices, which the program shows by printing the vocabulary size
line.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>

int main() {
  namespace nlp = pluss::ai::nlp;

  const nlp::runtime::EmbeddingTable table{nlp::runtime::EmbeddingOptions{.width = 4}};
  for (const std::string& line : table.dump_state_lines()) {
    if (line.rfind("vocab_size=", 0) == 0) std::cout << line << "\n";
  }
  return 0;
}
```

Output

```text
vocab_size=0
```

## See Also

- [`load_state_lines`](load_state_lines.md) -- restores these lines.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="forward.html">
    <span class="label">Next</span>
    <span class="title">forward</span>
  </a>
</div>
<!-- apiref:nav:end -->
