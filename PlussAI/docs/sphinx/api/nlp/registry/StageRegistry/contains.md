<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 8622eaf36c39 -->
# `pluss::ai::nlp::registry::StageRegistry::contains`

[Back to `pluss::ai::nlp::registry::StageRegistry`](../StageRegistry.md)

## Signature

```cpp
bool pluss::ai::nlp::registry::StageRegistry::contains(std::string_view name) const;
```

## What It Does

Reports whether `name` is registered. It is the read-only check to make before adding a factory or creating a stage from a name that came from outside the program.

## Parameters

`name` is the registry name to look up. The lookup is exact and case-sensitive.

## Returns

`true` when a factory is registered under `name`, `false` otherwise.

## Errors

None.

## Practical Examples

### Example 1

Checking the built-in names.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

namespace nlp = pluss::ai::nlp;

// A custom stage: it tags every token with a fixed label.
class LabelStage final : public nlp::IStage {
public:
  explicit LabelStage(std::string label = "X") : label_(std::move(label)) {}
  std::string_view name() const noexcept override { return "label"; }
  void apply(nlp::Doc& doc) const override {
    for (nlp::Token& token : doc.tokens) token.pos = label_;
  }
private:
  std::string label_;
};

int main() {
  const auto& catalog = nlp::registry::StageRegistry::instance();
  std::cout << "tokenizer name: " << catalog.contains("rule_tokenizer") << "\n";
  std::cout << "unknown name: " << catalog.contains("parser") << "\n";
  return 0;
}
```

Output

```text
tokenizer name: 1
unknown name: 0
```

### Example 2

The lookup is case-sensitive.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

namespace nlp = pluss::ai::nlp;

// A custom stage: it tags every token with a fixed label.
class LabelStage final : public nlp::IStage {
public:
  explicit LabelStage(std::string label = "X") : label_(std::move(label)) {}
  std::string_view name() const noexcept override { return "label"; }
  void apply(nlp::Doc& doc) const override {
    for (nlp::Token& token : doc.tokens) token.pos = label_;
  }
private:
  std::string label_;
};

int main() {
  const auto& catalog = nlp::registry::StageRegistry::instance();
  std::cout << "tagger = " << catalog.contains("tagger") << ", Tagger = " << catalog.contains("Tagger") << "\n";
  return 0;
}
```

Output

```text
tagger = 1, Tagger = 0
```

### Example 3

Checking before adding, so a registration is made only when the name is free.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

namespace nlp = pluss::ai::nlp;

// A custom stage: it tags every token with a fixed label.
class LabelStage final : public nlp::IStage {
public:
  explicit LabelStage(std::string label = "X") : label_(std::move(label)) {}
  std::string_view name() const noexcept override { return "label"; }
  void apply(nlp::Doc& doc) const override {
    for (nlp::Token& token : doc.tokens) token.pos = label_;
  }
private:
  std::string label_;
};

int main() {
  auto& catalog = nlp::registry::StageRegistry::instance();
  if (!catalog.contains("label")) catalog.add("label", [] { return std::make_shared<LabelStage>(); });
  std::cout << "label registered = " << catalog.contains("label") << "\n";
  return 0;
}
```

Output

```text
label registered = 1
```

## See Also

- [`add`](add.md) -- registers a name.
- [`create`](create.md) -- builds a registered name.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="add.html">
    <span class="label">Previous</span>
    <span class="title">add</span>
  </a>
  <a class="next-link" href="create.html">
    <span class="label">Next</span>
    <span class="title">create</span>
  </a>
</div>
<!-- apiref:nav:end -->
