<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f5b4721b815d -->
# `pluss::ai::nlp::registry::StageRegistry::instance`

[Back to `pluss::ai::nlp::registry::StageRegistry`](../StageRegistry.md)

## Signature

```cpp
pluss::ai::nlp::registry::StageRegistry& pluss::ai::nlp::registry::StageRegistry::instance();
```

## What It Does

Returns the process-wide catalog, creating it on first use with the eight built-in stages registered. Every call returns the same object, so a registration made through one reference is visible through all of them.

## Parameters

None.

## Returns

A reference to the single catalog. It stays valid for the life of the program.

## Errors

None.

## Practical Examples

### Example 1

Listing the built-in names of a fresh catalog.

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
  for (const std::string& name : nlp::registry::StageRegistry::instance().names()) std::cout << name << "\n";
  return 0;
}
```

Output

```text
lemmatizer
matcher
ner
rule_tokenizer
sentencizer
syntax
tagger
textcat
```

### Example 2

Checking that two references name the same catalog. A registration made through the first is visible through the second.

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
  auto& first = nlp::registry::StageRegistry::instance();
  auto& second = nlp::registry::StageRegistry::instance();
  first.add("label", [] { return std::make_shared<LabelStage>(); });
  std::cout << "same catalog = " << (&first == &second) << ", label visible = " << second.contains("label") << "\n";
  return 0;
}
```

Output

```text
same catalog = 1, label visible = 1
```

### Example 3

Checking a built-in name before and after the catalog is used.

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
  std::cout << "tagger registered = " << nlp::registry::StageRegistry::instance().contains("tagger") << "\n";
  return 0;
}
```

Output

```text
tagger registered = 1
```

## See Also

- [`add`](add.md) -- registers a factory on this catalog.
- [`names`](names.md) -- lists the catalog.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="create.html">
    <span class="label">Previous</span>
    <span class="title">create</span>
  </a>
  <a class="next-link" href="names.html">
    <span class="label">Next</span>
    <span class="title">names</span>
  </a>
</div>
<!-- apiref:nav:end -->
