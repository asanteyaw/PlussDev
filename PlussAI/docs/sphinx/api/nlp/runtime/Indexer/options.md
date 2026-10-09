<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: fa3b947aed06 -->
# `pluss::ai::nlp::runtime::Indexer::options`

[Back to `pluss::ai::nlp::runtime::Indexer`](../Indexer.md)

## Signature

```cpp
const pluss::ai::nlp::runtime::SequenceOptions& pluss::ai::nlp::runtime::Indexer::options() const noexcept;
```

## What It Does

Returns the options the indexer was constructed with, or restored with. Every batch the indexer builds
follows these options, so reading them tells a caller how the ids were produced.

## Parameters

None.

## Returns

A `const` reference to the [`SequenceOptions`](../SequenceOptions.md). It stays valid for the life of the
indexer, and it changes only when the indexer loads state.

## Errors

None. The method is `noexcept`.

## Practical Examples

Three examples: reading the default options, reading options set at construction, and the options after
restoring state.

### Example 1

The default options of a new indexer, printed as the flags that matter most for batching.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  const nlp::runtime::Indexer indexer{};
  std::cout << "use_lemmas = " << indexer.options().use_lemmas << ", skip_punct = " << indexer.options().skip_punct << "\n";
  std::cout << "add_bos = " << indexer.options().add_bos << ", batch_first = " << indexer.options().batch_first << "\n";
  return 0;
}
```

Output

```text
use_lemmas = 1, skip_punct = 1
add_bos = 0, batch_first = 1
```

### Example 2

Options set at construction. The boundary tokens and the sequence layout are read back, so the program can
confirm the configuration it asked for.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  const nlp::runtime::Indexer indexer{nlp::runtime::SequenceOptions{.add_bos = true, .batch_first = false}};
  std::cout << "add_bos = " << indexer.options().add_bos << ", batch_first = " << indexer.options().batch_first << "\n";
  std::cout << "bos_token = " << indexer.options().bos_token << "\n";
  return 0;
}
```

Output

```text
add_bos = 1, batch_first = 0
bos_token = <bos>
```

### Example 3

The options after restoring state. A saved indexer's options come back with its vocabulary, so the restored
indexer reports the same configuration as the original.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  nlp::runtime::Indexer original{nlp::runtime::SequenceOptions{.lowercase = false}};
  original.learn(wf.infer(std::vector<std::string>{"Ada."}));
  const auto lines = original.dump_state_lines();
  const std::vector<std::string_view> views(lines.begin(), lines.end());
  nlp::runtime::Indexer restored{};
  restored.load_state_lines(views);
  std::cout << "restored lowercase = " << restored.options().lowercase << "\n";
  return 0;
}
```

Output

```text
restored lowercase = 0
```

## See Also

- [`SequenceOptions`](../SequenceOptions.md) -- every field.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="load_state_lines.html">
    <span class="label">Previous</span>
    <span class="title">load_state_lines</span>
  </a>
  <a class="next-link" href="pad_id.html">
    <span class="label">Next</span>
    <span class="title">pad_id</span>
  </a>
</div>
<!-- apiref:nav:end -->
