<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 70a2c981144b -->

# `pluss::ai::nlp::IStage::learn`

[Back to `pluss::ai::nlp::IStage`](../IStage.md)

## Signature

```cpp
virtual void pluss::ai::nlp::IStage::learn(const std::vector<pluss::ai::nlp::Doc>& docs, const pluss::ai::nlp::TrainOptions& options);
virtual void pluss::ai::nlp::IStage::learn(const std::vector<pluss::ai::nlp::Doc>& docs, pluss::RangeView<std::string_view> labels, const pluss::ai::nlp::TrainOptions& options);
```

## What It Does

Lets a trainable stage read a batch of documents before inference. The default implementation does nothing, so a stage without learned state never needs to override it. A pipeline's `Workflow::learn` calls `learn` on every stage with the prepared documents. The second overload also receives one label string per document, for stages that learn from labels such as the text classifier.

## Parameters

`const std::vector<Doc>& docs`
: The training documents, already run through the stages that precede this one.

`pluss::RangeView<std::string_view> labels`
: One label per document, for the labeled overload only.

`const TrainOptions& options`
: Shared training settings, including the random seed and whether empty documents are allowed.

## Returns

Nothing (`void`). The stage records whatever it learns in its own fields.

## Errors

Implementation-defined. A stage should throw if the labels do not match the documents.

## Practical Examples

### Example 1

A pipeline's `learn` reaches a custom stage with the number of documents it was given.

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
  auto stage = std::make_shared<TagStage>("NOUN");
  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), stage}};
  wf.learn(std::vector<std::string>{"one", "two", "three"});
  std::cout << "seen=" << stage->seen_ << "\n";
  return 0;
}
```

Output

```text
seen=3
```

### Example 2

Training once and then inferring: the stage's `apply` sees the trained configuration.

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
  auto stage = std::make_shared<TagStage>("NOUN");
  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), stage}};
  wf.learn(std::vector<std::string>{"one", "two"});
  const nlp::Doc doc = wf.infer("Laurel writes");
  std::cout << "seen=" << stage->seen_ << " pos=" << doc.tokens[0].pos << "\n";
  return 0;
}
```

Output

```text
seen=2 pos=NOUN
```

### Example 3

A stage that never overrides `learn` simply ignores training.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

namespace nlp = pluss::ai::nlp;

int main() {
  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  wf.learn(std::vector<std::string>{"one", "two"});
  const nlp::Doc doc = wf.infer("Laurel writes");
  std::cout << "tokens=" << doc.tokens.size() << "\n";
  return 0;
}
```

Output

```text
tokens=2
```

## See Also

- [`apply`](apply.md) -- the inference counterpart.
- [`dump_state_lines`](dump_state_lines.md) -- saving what training produced.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dump_state_lines.html">
    <span class="label">Previous</span>
    <span class="title">dump_state_lines</span>
  </a>
  <a class="next-link" href="load_state_lines.html">
    <span class="label">Next</span>
    <span class="title">load_state_lines</span>
  </a>
</div>
<!-- apiref:nav:end -->
