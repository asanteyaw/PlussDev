<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 66515c52b42b -->

# `pluss::ai::nlp::IStage::load_state_lines`

[Back to `pluss::ai::nlp::IStage`](../IStage.md)

## Signature

```cpp
virtual void pluss::ai::nlp::IStage::load_state_lines(pluss::RangeView<std::string_view> lines);
```

## What It Does

Restores the stage from the state lines `dump_state_lines` wrote. A workflow's `load` calls it with the lines it saved for this stage, after constructing the stage through the registry. The default implementation rejects any non-empty state, so a stage that dumps state must override it, or loading the saved workflow fails with a clear error.

## Parameters

`pluss::RangeView<std::string_view> lines`
: The saved state lines for this stage, in the order they were written. A braced list of string literals works directly.

## Returns

Nothing (`void`). The stage's own fields reflect the restored state.

## Errors

The default implementation throws when given any lines. An override should throw on a line it cannot parse.

## Practical Examples

### Example 1

A fresh stage restored from a saved set of lines takes on the saved configuration.

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
  TagStage stage{"X"};
  stage.load_state_lines({"tag=alpha", "seen=7"});
  std::cout << "tag=" << stage.tag_ << " seen=" << stage.seen_ << "\n";
  return 0;
}
```

Output

```text
tag=alpha seen=7
```

### Example 2

Dumping and reloading round-trips the state exactly.

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
  TagStage source{"ADV"};
  source.seen_ = 5;
  TagStage restored{"X"};
  const std::vector<std::string> lines = source.dump_state_lines();
  std::vector<std::string_view> views(lines.begin(), lines.end());
  restored.load_state_lines(views);
  std::cout << "tag=" << restored.tag_ << " seen=" << restored.seen_ << "\n";
  return 0;
}
```

Output

```text
tag=ADV seen=5
```

### Example 3

A stage that keeps no state rejects any lines it is handed.

```cpp
#include <pluss/nlp.h>

#include <iostream>

namespace nlp = pluss::ai::nlp;

int main() {
  nlp::RuleTokenizerStage tokenizer{};
  bool threw = false;
  try {
    tokenizer.load_state_lines({"unexpected=1"});
  } catch (const std::exception&) {
    threw = true;
  }
  std::cout << "threw=" << threw << "\n";
  return 0;
}
```

Output

```text
threw=1
```

## See Also

- [`dump_state_lines`](dump_state_lines.md) -- the writing counterpart.
- [`registry`](../../registry.md) -- constructing the stage before its state is restored.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="learn.html">
    <span class="label">Previous</span>
    <span class="title">learn</span>
  </a>
  <a class="next-link" href="name.html">
    <span class="label">Next</span>
    <span class="title">name</span>
  </a>
</div>
<!-- apiref:nav:end -->
