<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 69a672b4200b -->
# `pluss::ai::ice::CallbackChain::add`

[Back to `pluss::ai::ice::CallbackChain`](../CallbackChain.md)

## Signature

```cpp
void pluss::ai::ice::CallbackChain::add(std::shared_ptr<pluss::ai::ice::Callback> callback);
```

## What It Does

Appends `callback` to the end of this chain's ordered list -- the next
time any hook fires, `callback` receives it after every callback
already in the chain. A `nullptr` argument is silently skipped rather
than stored, so a chain's `callbacks()` never holds a null entry to
guard against later.

## Parameters

`std::shared_ptr<Callback> callback`
: The callback to append. Shared ownership -- the same `Callback`
  instance can be held by more than one chain, or kept alive by the
  caller independently of the chain, at the same time.

## Returns

`void` -- mutates this chain in place.

## Errors

None -- `nullptr` is accepted and simply ignored (see Example 2).

## Practical Examples

Three examples: appending two callbacks and confirming the count,
confirming a null argument is skipped, and the real pattern this exists
for -- grouping several callbacks into one chain so `Coach::add_callback`
can take them as a single argument.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>
#include <memory>

namespace {

class NoopCallback final : public pluss::ai::ice::Callback {};

} // namespace

int main() {
  namespace ice = pluss::ai::ice;

  ice::CallbackChain chain;
  std::cout << "before: callbacks().size() = " << chain.callbacks().size() << "\n";

  chain.add(std::make_shared<NoopCallback>());
  chain.add(std::make_shared<NoopCallback>());
  std::cout << "after:  callbacks().size() = " << chain.callbacks().size() << "\n";

  return 0;
}
```

Output

```text
before: callbacks().size() = 0
after:  callbacks().size() = 2
```

### Example 2

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>
#include <memory>

namespace {

class NoopCallback final : public pluss::ai::ice::Callback {};

} // namespace

int main() {
  namespace ice = pluss::ai::ice;

  ice::CallbackChain chain;
  chain.add(std::make_shared<NoopCallback>());

  // A null callback is silently skipped, not stored as an empty slot.
  chain.add(nullptr);
  std::cout << "callbacks().size() after add(nullptr) = " << chain.callbacks().size() << "\n";

  return 0;
}
```

Output

```text
callbacks().size() after add(nullptr) = 1
```

### Example 3

`CallbackChain` is itself a `Callback`, so a chain built with `add` can
be handed to [`Coach::add_callback`](../../coach/Coach.md) as a single
argument -- both callbacks fire, in the order they were added to the
chain.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>
#include <memory>

namespace {

class TagCallback final : public pluss::ai::ice::Callback {
 public:
  explicit TagCallback(std::string tag) : tag_(std::move(tag)) {}
  void on_run_begin(pluss::ai::ice::RunState&) override {
    std::cout << "on_run_begin from " << tag_ << "\n";
  }

 private:
  std::string tag_;
};

} // namespace

int main() {
  namespace ice = pluss::ai::ice;
  namespace nn = pluss::nn;

  auto chain = std::make_shared<ice::CallbackChain>();
  chain->add(std::make_shared<TagCallback>("first"));
  chain->add(std::make_shared<TagCallback>("second"));

  ice::Outline model(nn::Fusion(2, 1));
  const pluss::Series x = pluss::make_series({0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {4, 2});
  const pluss::Series y = pluss::make_series({0.0f, 1.0f, 1.0f, 2.0f}, {4, 1});

  ice::TrainOptions options{};
  options.execution.max_epochs = 1;
  options.learning_rate = 0.1;
  options.validation.split_ratio = 0.0;

  ice::Coach coach(options);
  coach.add_callback(chain);
  coach.learn(model, x, y);

  return 0;
}
```

Output

```text
on_run_begin from first
on_run_begin from second
```

## See Also

- [`callbacks`](callbacks.md) -- read the list this appends to.
- [`Coach::add_callback`](../../coach/Coach.md) -- takes any single
  `Callback`, including a whole chain.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="callbacks.html">
    <span class="label">Next</span>
    <span class="title">callbacks</span>
  </a>
</div>
<!-- apiref:nav:end -->
