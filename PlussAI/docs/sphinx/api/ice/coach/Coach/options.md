<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: fde31f78ca5b -->
# `pluss::ai::ice::Coach::options`

[Back to `pluss::ai::ice::Coach`](../Coach.md)

## Signature

```cpp
[[nodiscard]] const pluss::ai::ice::TrainOptions& pluss::ai::ice::Coach::options() const noexcept;
```

## What It Does

Returns the [`TrainOptions`](../../options/TrainOptions.md) this
`Coach` was constructed with (or default-constructed, if none was
given) -- the exact value every [`learn`](learn.md)/
[`infer`](infer.md)/[`assess`](assess.md) call reads from. The same
values are reachable from inside a running
[`Callback`](../../callback/Callback.md) through
[`RunState::recipe`](../../callback/RunState.md#recipe).

## Parameters

None.

## Returns

`const TrainOptions&` -- a reference into this `Coach`'s own storage,
fixed for the object's lifetime (there is no setter; build a new
`Coach` to train with different options).

## Errors

None.

## Practical Examples

Three examples: the default-constructed value, a value passed at
construction, and confirming it matches what a running callback reads
through `RunState::recipe`.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  const ice::Coach coach;
  std::cout << "options().optimizer == Adam: " << (coach.options().optimizer == ice::OptimizerKind::Adam) << "\n";
  std::cout << "options().learning_rate = " << coach.options().learning_rate << "\n";
  std::cout << "options().execution.max_epochs = " << coach.options().execution.max_epochs << "\n";

  return 0;
}
```

Output

```text
options().optimizer == Adam: 1
options().learning_rate = 0.001
options().execution.max_epochs = 60
```

### Example 2

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  ice::TrainOptions options{};
  options.optimizer = ice::OptimizerKind::SGD;
  options.learning_rate = 0.25;

  const ice::Coach coach(options);
  std::cout << "options().optimizer == SGD: " << (coach.options().optimizer == ice::OptimizerKind::SGD) << "\n";
  std::cout << "options().learning_rate = " << coach.options().learning_rate << "\n";

  return 0;
}
```

Output

```text
options().optimizer == SGD: 1
options().learning_rate = 0.25
```

### Example 3

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>
#include <memory>

namespace {

class LogRecipeCallback final : public pluss::ai::ice::Callback {
 public:
  void on_run_begin(pluss::ai::ice::RunState& state) override {
    std::cout << "state.recipe->learning_rate = " << state.recipe->learning_rate << "\n";
  }
};

} // namespace

int main() {
  namespace ice = pluss::ai::ice;
  namespace nn = pluss::nn;

  ice::TrainOptions options{};
  options.learning_rate = 0.25;
  options.execution.max_epochs = 1;
  options.validation.split_ratio = 0.0;

  ice::Outline model(nn::Fusion(2, 1));
  const pluss::Series x = pluss::make_series({0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {4, 2});
  const pluss::Series y = pluss::make_series({0.0f, 1.0f, 1.0f, 2.0f}, {4, 1});

  ice::Coach coach(options);
  coach.add_callback(std::make_shared<LogRecipeCallback>());
  std::cout << "coach.options().learning_rate = " << coach.options().learning_rate << "\n";
  coach.learn(model, x, y);

  return 0;
}
```

Output

```text
coach.options().learning_rate = 0.25
state.recipe->learning_rate = 0.25
```

## See Also

- [`TrainOptions`](../../options/TrainOptions.md) -- every field this
  returns.
- [`RunState::recipe`](../../callback/RunState.md#recipe) -- the same
  value, reached from inside a running callback.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="learn.html">
    <span class="label">Previous</span>
    <span class="title">learn</span>
  </a>
</div>
<!-- apiref:nav:end -->
