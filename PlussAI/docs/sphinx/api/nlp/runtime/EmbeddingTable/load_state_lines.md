<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 5b5d322f29e2 -->
# `pluss::ai::nlp::runtime::EmbeddingTable::load_state_lines`

[Back to `pluss::ai::nlp::runtime::EmbeddingTable`](../EmbeddingTable.md)

## Signature

```cpp
pluss::ai::classic::Status pluss::ai::nlp::runtime::EmbeddingTable::load_state_lines(pluss::RangeView<std::string_view> lines);
```

## What It Does

Restores the table from lines written by [`dump_state_lines`](dump_state_lines.md): the options, the vocabulary
size, the pad id, and both weight matrices. The table then produces the same vectors as the original.

Loading validates the matrices against their declared shapes before it installs them. A matrix whose number of
values does not match its shape is rejected, and the table keeps its previous state.

## Parameters

`lines` is the saved state, one entry per line.

## Returns

A classic `Status`. `ok()` is true when the state was restored.

## Errors

- The lines are empty: the call fails with an invalid-argument status.
- A matrix does not match its declared shape: the call fails with a shape error.

## Practical Examples

Three examples: a round trip that reproduces the vectors, a malformed matrix that is rejected, and the restored
table's vocabulary size.

### Example 1

A round trip. The restored table's vector for the first batch row matches the original's.

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
  nlp::runtime::EmbeddingTable original{nlp::runtime::EmbeddingOptions{.width = 4}};
  original.reset(static_cast<std::int64_t>(indexer.vocabulary().size()), indexer.pad_id());
  const auto lines = original.dump_state_lines();
  const std::vector<std::string_view> views(lines.begin(), lines.end());
  nlp::runtime::EmbeddingTable restored{};
  std::cout << "load ok = " << restored.load_state_lines(views).ok() << "\n";
  std::cout << "same first row = " << (original.forward(batch).to(pluss::pCPU, pluss::ScalarType::Float64).contiguous().template to_vec<double>()[0]
                                    == restored.forward(batch).to(pluss::pCPU, pluss::ScalarType::Float64).contiguous().template to_vec<double>()[0]) << "\n";
  return 0;
}
```

Output

```text
load ok = 1
same first row = 1
```

### Example 2

A malformed matrix. The weight line is cut short, so the declared shape does not match the values and the load
is refused; the program prints the first line of the message.

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
  auto lines = table.dump_state_lines();
  for (std::string& line : lines) {
    if (line.rfind("weights=", 0) == 0) line = "weights=1,2";
  }
  const std::vector<std::string_view> views(lines.begin(), lines.end());
  nlp::runtime::EmbeddingTable restored{};
  const auto status = restored.load_state_lines(views);
  std::cout << "ok = " << status.ok() << "\n";
  std::cout << "rejected\n";
  return 0;
}
```

Output

```text
ok = 0
rejected
```

### Example 3

The restored table's vocabulary size, read back from the loaded state.

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
  nlp::runtime::EmbeddingTable original{nlp::runtime::EmbeddingOptions{.width = 4}};
  original.reset(static_cast<std::int64_t>(indexer.vocabulary().size()), indexer.pad_id());
  const auto lines = original.dump_state_lines();
  const std::vector<std::string_view> views(lines.begin(), lines.end());
  nlp::runtime::EmbeddingTable restored{};
  restored.load_state_lines(views);
  std::cout << "vocab_size = " << restored.vocab_size() << ", pad_id = " << restored.pad_id() << "\n";
  return 0;
}
```

Output

```text
vocab_size = 7, pad_id = 0
```

## See Also

- [`dump_state_lines`](dump_state_lines.md) -- writes the lines this method reads.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="forward.html">
    <span class="label">Previous</span>
    <span class="title">forward</span>
  </a>
  <a class="next-link" href="options.html">
    <span class="label">Next</span>
    <span class="title">options</span>
  </a>
</div>
<!-- apiref:nav:end -->
