<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: de0be1bbef04 -->

# `pluss::ai::nlp::IStage::dump_state_lines`

[Back to `pluss::ai::nlp::IStage`](../IStage.md)

## Signature

```cpp
virtual std::vector<std::string> pluss::ai::nlp::IStage::dump_state_lines() const;
```

## What It Does

Returns the stage's persisted state as text lines, one entry per line. A workflow's `save` writes these lines for every stage, and `load` hands them back through `load_state_lines`. The default returns no lines, which is right for a stateless stage. Keep each line self-describing, such as `key=value`, so a later version can read it back.

## Parameters

None.

## Returns

A `std::vector<std::string>` of state lines. Empty for a stateless stage.

## Errors

Implementation-defined. Lines should contain no embedded newlines.

## Practical Examples

### Example 1

A stage's state comes back as the lines it chose to write.

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
  TagStage stage{"ADJ"};
  for (const std::string& line : stage.dump_state_lines()) {
    std::cout << line << "\n";
  }
  return 0;
}
```

Output

```text
tag=ADJ
seen=0
```

### Example 2

State reflects training: the `seen` line changes after `learn`.

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
  auto stage = std::make_shared<TagStage>("ADJ");
  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), stage}};
  wf.learn(std::vector<std::string>{"one", "two", "three", "four"});
  for (const std::string& line : stage->dump_state_lines()) {
    std::cout << line << "\n";
  }
  return 0;
}
```

Output

```text
tag=ADJ
seen=4
```

### Example 3

A custom stage that does not override `dump_state_lines` inherits the empty
default, so it has nothing to save.

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
  std::cout << "lines=" << stage.dump_state_lines().size() << "\n";
  return 0;
}
```

Output

```text
lines=0
```

## See Also

- [`load_state_lines`](load_state_lines.md) -- the reading counterpart.
- [`learn`](learn.md) -- what produces the state.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="apply.html">
    <span class="label">Previous</span>
    <span class="title">apply</span>
  </a>
  <a class="next-link" href="learn.html">
    <span class="label">Next</span>
    <span class="title">learn</span>
  </a>
</div>
<!-- apiref:nav:end -->
