<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ea519fcb4db9 -->
# `pluss::ai::nlp::registry::StageRegistry::create`

[Back to `pluss::ai::nlp::registry::StageRegistry`](../StageRegistry.md)

## Signature

```cpp
pluss::ai::nlp::Stage pluss::ai::nlp::registry::StageRegistry::create(std::string_view name) const;
```

## What It Does

Builds a stage from the factory registered under `name`. The stage comes back in its default configuration: its options and learned state are not restored here. Restoring them is the job of the stage's state lines, applied afterward.

## Parameters

`name` is a registered name, such as one read from a saved workflow.

## Returns

A `Stage` built by the registered factory.

## Errors

- `name` is not registered: the call fails with a precondition error that names it.

## Practical Examples

### Example 1

Creating a built-in stage and reading its name.

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
  const nlp::Stage stage = nlp::registry::StageRegistry::instance().create("sentencizer");
  std::cout << "created = " << stage->name() << "\n";
  return 0;
}
```

Output

```text
created = sentencizer
```

### Example 2

Creating a custom stage. The factory is registered first, then the stage is built from the name alone.

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
  catalog.add("label", [] { return std::make_shared<LabelStage>("Z"); });
  const nlp::Stage stage = catalog.create("label");
  std::cout << "created = " << stage->name() << "\n";
  return 0;
}
```

Output

```text
created = label
```

### Example 3

An unknown name. The call fails, and the program prints the first line of the message.

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
  try {
    nlp::registry::StageRegistry::instance().create("no_such_stage");
    std::cout << "created\n";
  } catch (const std::exception& error) {
    std::cout << "rejected\n";
  }
  return 0;
}
```

Output

```text
rejected
```

## See Also

- [`add`](add.md) -- registers the factory this method calls.
- [`contains`](contains.md) -- checks a name first.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="contains.html">
    <span class="label">Previous</span>
    <span class="title">contains</span>
  </a>
  <a class="next-link" href="instance.html">
    <span class="label">Next</span>
    <span class="title">instance</span>
  </a>
</div>
<!-- apiref:nav:end -->
