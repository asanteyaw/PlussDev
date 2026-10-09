<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 57cbcdbb3522 -->
# `pluss::ai::nlp::registry::StageRegistry::names`

[Back to `pluss::ai::nlp::registry::StageRegistry`](../StageRegistry.md)

## Signature

```cpp
std::vector<std::string> pluss::ai::nlp::registry::StageRegistry::names() const;
```

## What It Does

Returns every registered name in sorted order. The list is a snapshot: a name registered after the call is not in it.

## Parameters

None.

## Returns

A `std::vector<std::string>` holding the registered names in ascending order.

## Errors

None.

## Practical Examples

### Example 1

The sorted names of a fresh catalog.

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

The count before and after a custom registration.

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
  const std::size_t before = catalog.names().size();
  catalog.add("label", [] { return std::make_shared<LabelStage>(); });
  std::cout << "before = " << before << ", after = " << catalog.names().size() << "\n";
  return 0;
}
```

Output

```text
before = 8, after = 9
```

### Example 3

The sorted order is independent of registration order. A name registered first can still sort after a name registered later.

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
  catalog.add("zeta", [] { return std::make_shared<LabelStage>(); });
  catalog.add("alpha", [] { return std::make_shared<LabelStage>(); });
  const auto names = catalog.names();
  std::cout << "first = " << names.front() << ", last = " << names.back() << "\n";
  return 0;
}
```

Output

```text
first = alpha, last = zeta
```

## See Also

- [`contains`](contains.md) -- checks a single name.
- [`create`](create.md) -- builds one of the listed names.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="instance.html">
    <span class="label">Previous</span>
    <span class="title">instance</span>
  </a>
</div>
<!-- apiref:nav:end -->
