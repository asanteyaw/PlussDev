<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: fadb87dd75d2 -->
# `pluss::ai::ice::Coach::callbacks`

[Back to `pluss::ai::ice::Coach`](../Coach.md)

## Signature

```cpp
[[nodiscard]] const std::vector<std::shared_ptr<pluss::ai::ice::Callback>>&
pluss::ai::ice::Coach::callbacks() const noexcept;
```

## What It Does

Returns the callbacks added to this `Coach` -- via its own
[two-argument constructor](../Coach.md) or
[`add_callback`](add_callback.md) -- in the order they were added.
This list is exactly what gets passed into
[`learn`](learn.md)/[`infer`](infer.md)/[`assess`](assess.md); it never
includes a built-in [`EarlyStopCallback`](../../callback/EarlyStopCallback.md)
`learn` adds automatically under its own conditions -- that instance
lives only inside the
[`CallbackChain`](../../callback/CallbackChain.md) `learn` assembles
for one call, and is never written back here.

## Parameters

None.

## Returns

`const std::vector<std::shared_ptr<Callback>>&` -- a reference into
this `Coach`'s own storage.

## Errors

None.

## Practical Examples

Three examples: empty before any callback is added, populated through
the two-argument constructor directly, and confirming the auto-added
`EarlyStopCallback` never shows up here even after a `learn` call that
triggers it.

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

  ice::Coach coach;
  std::cout << "before: callbacks().size() = " << coach.callbacks().size() << "\n";

  coach.add_callback(std::make_shared<NoopCallback>());
  std::cout << "after:  callbacks().size() = " << coach.callbacks().size() << "\n";

  return 0;
}
```

Output

```text
before: callbacks().size() = 0
after:  callbacks().size() = 1
```

### Example 2

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>
#include <memory>
#include <vector>

namespace {

class NoopCallback final : public pluss::ai::ice::Callback {};

} // namespace

int main() {
  namespace ice = pluss::ai::ice;

  std::vector<std::shared_ptr<ice::Callback>> seeded{
      std::make_shared<NoopCallback>(), std::make_shared<NoopCallback>()};

  const ice::Coach coach(ice::TrainOptions{}, seeded);
  std::cout << "callbacks().size() = " << coach.callbacks().size() << "\n";

  return 0;
}
```

Output

```text
callbacks().size() = 2
```

### Example 3

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;
  namespace nn = pluss::nn;

  ice::Outline model(nn::Fusion(2, 1));
  const pluss::Series x = pluss::make_series({0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {4, 2});
  const pluss::Series y = pluss::make_series({0.0f, 1.0f, 1.0f, 2.0f}, {4, 1});

  ice::TrainOptions options{};
  options.execution.max_epochs = 1;
  options.validation.split_ratio = 0.5;  // triggers an auto-added EarlyStopCallback

  ice::Coach coach(options);
  std::cout << "before learn: callbacks().size() = " << coach.callbacks().size() << "\n";
  coach.learn(model, x, y);

  // The auto-added EarlyStopCallback lives only in the CallbackChain Coach
  // assembles internally for that one call -- it never touches
  // Coach::callbacks() itself.
  std::cout << "after learn:  callbacks().size() = " << coach.callbacks().size() << "\n";

  return 0;
}
```

Output

```text
before learn: callbacks().size() = 0
after learn:  callbacks().size() = 0
```

## See Also

- [`add_callback`](add_callback.md) -- the only way entries get added
  (besides the constructor).
- [`EarlyStopCallback`](../../callback/EarlyStopCallback.md) -- the
  built-in callback that never shows up here.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="assess.html">
    <span class="label">Previous</span>
    <span class="title">assess</span>
  </a>
  <a class="next-link" href="infer.html">
    <span class="label">Next</span>
    <span class="title">infer</span>
  </a>
</div>
<!-- apiref:nav:end -->
