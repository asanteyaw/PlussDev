<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c6659f186b47 -->

# `pluss::ai::nlp::IStage::name`

[Back to `pluss::ai::nlp::IStage`](../IStage.md)

## Signature

```cpp
virtual std::string_view pluss::ai::nlp::IStage::name() const noexcept = 0;
```

## What It Does

Returns the stage's registry name. The name is what a saved workflow
records for each stage, and what [`registry::StageRegistry`](../../registry/StageRegistry.md)
uses to build the stage again by name during `Workflow::load`. It must be
stable: a stage keeps the same name across builds, or previously saved
workflows stop loading. The method is `noexcept` and cheap, so a caller can
ask for it freely, for logging or for matching a stage to its saved state.

## Parameters

None.

## Returns

A `std::string_view` naming the stage. Copy it into a `std::string` if it
must outlive the stage object that produced it.

## Errors

None. The method does not throw.

## Practical Examples

### Example 1

Asking a built-in stage for its registry name.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::RuleTokenizerStage tokenizer{};
  std::cout << "name=" << tokenizer.name() << "\n";
  return 0;
}
```

Output

```text
name=rule_tokenizer
```

### Example 2

A custom stage chooses its own name, which is what a saved workflow records
for it.

```cpp
#include <pluss/nlp.h>

#include <iostream>

namespace nlp = pluss::ai::nlp;

class Bare final : public nlp::IStage {
 public:
  std::string_view name() const noexcept override { return "bare"; }
  void apply(nlp::Doc&) const override {}
};

int main() {
  const Bare stage{};
  std::cout << "name=" << stage.name() << "\n";
  return 0;
}
```

Output

```text
name=bare
```

### Example 3

Two different built-in stages report different names, so a name identifies a
stage type rather than an instance.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  const nlp::SentencizerStage sentencizer{};
  const nlp::LemmatizerStage lemmatizer{};
  std::cout << "sentencizer=" << sentencizer.name() << "\n";
  std::cout << "lemmatizer=" << lemmatizer.name() << "\n";
  return 0;
}
```

Output

```text
sentencizer=sentencizer
lemmatizer=lemmatizer
```

## See Also

- [`registry::StageRegistry`](../../registry/StageRegistry.md) -- resolves
  these names back to stages.
- [`IStage`](../IStage.md) -- the contract this method belongs to.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="load_state_lines.html">
    <span class="label">Previous</span>
    <span class="title">load_state_lines</span>
  </a>
</div>
<!-- apiref:nav:end -->
