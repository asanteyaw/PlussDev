<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 2969e0157cc7 -->

# `pluss::ai::nlp::IStage::apply`

[Back to `pluss::ai::nlp::IStage`](../IStage.md)

## Signature

```cpp
virtual void pluss::ai::nlp::IStage::apply(pluss::ai::nlp::Doc& doc) const = 0;
```

## What It Does

Transforms one document in place. A pipeline calls `apply` on every document, once per stage, in stage order. An implementation reads what earlier stages wrote and writes its own annotations into the same `Doc`. It must not depend on other documents in the batch, since inference processes each input on its own.

## Parameters

`Doc& doc`
: The document to annotate. Write only the fields this stage owns, so later stages see a predictable result.

## Returns

Nothing (`void`). The result is the mutated `doc`.

## Errors

Implementation-defined. A stage should throw on an input it cannot process rather than silently leave the document half-annotated.

## Practical Examples

### Example 1

A custom stage's `apply` writes into every token of a tokenized document.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace nlp = pluss::ai::nlp;

class TagStage final : public nlp::IStage {
 public:
  explicit TagStage(std::string tag) : tag_(std::move(tag)) {}
  std::string_view name() const noexcept override { return "tag"; }
  void apply(nlp::Doc& doc) const override {
    for (nlp::Token& token : doc.tokens) token.pos = tag_;
  }
  void learn(const std::vector<nlp::Doc>& docs, const nlp::TrainOptions&) override { seen_ += static_cast<int>(docs.size()); }
  std::vector<std::string> dump_state_lines() const override { return {"tag=" + tag_, "seen=" + std::to_string(seen_)}; }
  void load_state_lines(pluss::RangeView<std::string_view> lines) override {
    for (const std::string_view line : lines) {
      if (line.rfind("tag=", 0) == 0) tag_ = std::string(line.substr(4));
      if (line.rfind("seen=", 0) == 0) seen_ = std::stoi(std::string(line.substr(5)));
    }
  }
  std::string tag_;
  int seen_ = 0;
};

int main() {
  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), std::make_shared<TagStage>("NOUN")}};
  const nlp::Doc doc = wf.infer("Laurel writes");
  std::cout << "first_pos=" << doc.tokens[0].pos << "\n";
  std::cout << "last_pos=" << doc.tokens[1].pos << "\n";
  return 0;
}
```

Output

```text
first_pos=NOUN
last_pos=NOUN
```

### Example 2

Calling `apply` directly on a document, outside any pipeline.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace nlp = pluss::ai::nlp;

class TagStage final : public nlp::IStage {
 public:
  explicit TagStage(std::string tag) : tag_(std::move(tag)) {}
  std::string_view name() const noexcept override { return "tag"; }
  void apply(nlp::Doc& doc) const override {
    for (nlp::Token& token : doc.tokens) token.pos = tag_;
  }
  void learn(const std::vector<nlp::Doc>& docs, const nlp::TrainOptions&) override { seen_ += static_cast<int>(docs.size()); }
  std::vector<std::string> dump_state_lines() const override { return {"tag=" + tag_, "seen=" + std::to_string(seen_)}; }
  void load_state_lines(pluss::RangeView<std::string_view> lines) override {
    for (const std::string_view line : lines) {
      if (line.rfind("tag=", 0) == 0) tag_ = std::string(line.substr(4));
      if (line.rfind("seen=", 0) == 0) seen_ = std::stoi(std::string(line.substr(5)));
    }
  }
  std::string tag_;
  int seen_ = 0;
};

int main() {
  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  nlp::Doc doc = wf.infer("Laurel writes");
  TagStage stage{"VERB"};
  stage.apply(doc);
  std::cout << "pos=" << doc.tokens[1].pos << "\n";
  return 0;
}
```

Output

```text
pos=VERB
```

### Example 3

Stages run in order, so a later stage overwrites what an earlier one wrote.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace nlp = pluss::ai::nlp;

class TagStage final : public nlp::IStage {
 public:
  explicit TagStage(std::string tag) : tag_(std::move(tag)) {}
  std::string_view name() const noexcept override { return "tag"; }
  void apply(nlp::Doc& doc) const override {
    for (nlp::Token& token : doc.tokens) token.pos = tag_;
  }
  void learn(const std::vector<nlp::Doc>& docs, const nlp::TrainOptions&) override { seen_ += static_cast<int>(docs.size()); }
  std::vector<std::string> dump_state_lines() const override { return {"tag=" + tag_, "seen=" + std::to_string(seen_)}; }
  void load_state_lines(pluss::RangeView<std::string_view> lines) override {
    for (const std::string_view line : lines) {
      if (line.rfind("tag=", 0) == 0) tag_ = std::string(line.substr(4));
      if (line.rfind("seen=", 0) == 0) seen_ = std::stoi(std::string(line.substr(5)));
    }
  }
  std::string tag_;
  int seen_ = 0;
};

int main() {
  nlp::Workflow wf{
      .lang = nlp::Language::English,
      .stages = {nlp::stage::tokenize(), std::make_shared<TagStage>("NOUN"), std::make_shared<TagStage>("VERB")},
  };
  const nlp::Doc doc = wf.infer("Laurel writes");
  std::cout << "pos=" << doc.tokens[0].pos << "\n";
  return 0;
}
```

Output

```text
pos=VERB
```

## See Also

- [`IStage`](../IStage.md) -- the contract this method belongs to.
- [`learn`](learn.md) -- the training counterpart.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="dump_state_lines.html">
    <span class="label">Next</span>
    <span class="title">dump_state_lines</span>
  </a>
</div>
<!-- apiref:nav:end -->
