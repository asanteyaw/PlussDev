<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ad5228f5ce23 -->
# `pluss::ai::nlp::registry::StageRegistry::add`

[Back to `pluss::ai::nlp::registry::StageRegistry`](../StageRegistry.md)

## Signature

```cpp
bool pluss::ai::nlp::registry::StageRegistry::add(std::string_view name, pluss::ai::nlp::registry::StageFactory factory);
```

## What It Does

Registers `factory` under `name`, so a workflow that names this stage can be loaded. A name that is already present is not replaced: the call returns false and the existing factory stays. This is what keeps a built-in name safe from being overwritten.

## Parameters

`name` is the stage's registry name, which must match what its `name()` returns for the saved workflow to load correctly.

`factory` is a zero-argument callable that returns a `Stage`, normally a `std::make_shared` of the stage in its default configuration.

## Returns

`true` when the name was new and the factory was stored; `false` when the name was already registered.

## Errors

- `name` is empty, or `factory` is empty: the call fails with a precondition error.

## Practical Examples

### Example 1

Registering a custom stage and building it by name.

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
  const bool added = catalog.add("label", [] { return std::make_shared<LabelStage>(); });
  std::cout << "added = " << added << ", contains = " << catalog.contains("label") << "\n";
  return 0;
}
```

Output

```text
added = 1, contains = 1
```

### Example 2

A duplicate registration. The second call returns false and the first factory stays in place, which the program shows by building the stage and reading the factory's label.

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
  catalog.add("label", [] { return std::make_shared<LabelStage>("first"); });
  const bool again = catalog.add("label", [] { return std::make_shared<LabelStage>("second"); });
  std::cout << "second add = " << again << "\n";
  std::cout << "created name = " << catalog.create("label")->name() << "\n";
  return 0;
}
```

Output

```text
second add = 0
created name = label
```

### Example 3

Refusing an empty name. The program prints the first line of the message.

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
  try {
    catalog.add("", [] { return std::make_shared<LabelStage>(); });
    std::cout << "added\n";
  } catch (const std::exception& error) {
    const std::string message = error.what();
    std::cout << message.substr(0, message.find('\n')) << "\n";
  }
  return 0;
}
```

Output

```text
nlp::registry::StageRegistry::add: a non-empty name and a valid factory are required.
```

## See Also

- [`create`](create.md) -- builds a stage from a registered name.
- [`contains`](contains.md) -- checks a name before adding.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="contains.html">
    <span class="label">Next</span>
    <span class="title">contains</span>
  </a>
</div>
<!-- apiref:nav:end -->
