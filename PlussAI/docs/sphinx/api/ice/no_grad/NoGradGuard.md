# `pluss::ai::ice::NoGradGuard`

An `ice`-local RAII no-grad scope, kept for source compatibility with
code written before `ice` and `pluss::ai::nn` shared one no-grad gate.
Constructing one disables the same process-wide, thread-local grad-mode
switch [`nn::NoGradGuard`](../../nn/grad/NoGradGuard.md) disables; the
destructor restores whatever was active before. Any `Series` operation
run while a `NoGradGuard` of either type is alive produces an untracked
result -- no computation node is recorded, so there is nothing to
reverse through -- regardless of whether the operands themselves track
their slope.

This is exactly what [`Model::infer`](../model/Model/infer.md) and
[`Model::assess`](../model/Model/assess.md) rely on: neither method
constructs one itself -- [`Coach`](../coach/Coach.md) wraps the
whole inference/assessment pass (and `Coach::learn`'s own validation
pass) in one, so every prediction a caller gets back from `infer`/
`assess` is already untracked, with no extra code at the call site. See
Example 3 below for the effect this has, and
[`Coach`'s own page](../coach/Coach.md) for exactly where the guard
is constructed.

## Signature

```cpp
pluss::ai::ice::NoGradGuard::NoGradGuard() noexcept = default;
template <typename Range> explicit pluss::ai::ice::NoGradGuard::NoGradGuard(const Range&) noexcept;
pluss::ai::ice::NoGradGuard::~NoGradGuard() = default;

pluss::ai::ice::NoGradGuard::NoGradGuard(const NoGradGuard&) = delete;
pluss::ai::ice::NoGradGuard& pluss::ai::ice::NoGradGuard::operator=(const NoGradGuard&) = delete;
pluss::ai::ice::NoGradGuard::NoGradGuard(NoGradGuard&&) = delete;
pluss::ai::ice::NoGradGuard& pluss::ai::ice::NoGradGuard::operator=(NoGradGuard&&) = delete;
```

## Construction

```cpp
const ice::NoGradGuard guard;  // disables recording for this scope
```

Both constructors have an identical effect: default-construct the
owned [`nn::NoGradGuard`](../../nn/grad/NoGradGuard.md) member, which
immediately disables grad-mode recording. The templated overload exists
only so a call site written against an older contract -- one that used
to pass a specific list of parameters to scope the guard to -- still
compiles unchanged; the argument is accepted and never read, because
the underlying switch has always been process-wide, not scoped to
particular parameters. Prefer the default constructor in new code; see
Example 2 for the compatibility overload in use.

Neither copyable nor movable, for the same reason as the type it wraps:
it is meant to guard exactly the scope it is declared in.

## State

None directly -- it holds one private
[`nn::NoGradGuard`](../../nn/grad/NoGradGuard.md) member, which is
where the actual previous-value bookkeeping lives.

## Methods

None besides the constructor/destructor -- its entire effect is its
lifetime.

## Practical Examples

Three examples: the basic disable-and-restore (observed through
`Series::source()`, since a value produced with recording disabled has
none), the compatibility constructor overload, and confirming
`Model::infer`'s predictions are already untracked without the caller
doing anything.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;
  namespace nn = pluss::nn;

  ice::Outline model(nn::Fusion(2, 1));
  const pluss::Series x = pluss::make_series({1.0f, 2.0f}, {1, 2});

  const pluss::Series y_tracked = model.forward(x);
  std::cout << "before guard, forward(x).source().has_value() = " << y_tracked.source().has_value() << "\n";

  {
    const ice::NoGradGuard guard;
    const pluss::Series y_no_grad = model.forward(x);
    std::cout << "inside guard, forward(x).source().has_value() = " << y_no_grad.source().has_value() << "\n";
  }

  const pluss::Series y_after = model.forward(x);
  std::cout << "after guard, forward(x).source().has_value() = " << y_after.source().has_value() << "\n";

  return 0;
}
```

Output

```text
before guard, forward(x).source().has_value() = 1
inside guard, forward(x).source().has_value() = 0
after guard, forward(x).source().has_value() = 1
```

### Example 2

The templated constructor accepts any range-like argument and ignores
it entirely -- the effect is identical to the default constructor
either way. This overload exists only so a call site that used to pass
a parameter list still compiles.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>
#include <vector>

int main() {
  namespace ice = pluss::ai::ice;
  namespace nn = pluss::nn;

  ice::Outline model(nn::Fusion(2, 1));
  const pluss::Series x = pluss::make_series({1.0f, 2.0f}, {1, 2});

  const std::vector<pluss::Series*> legacy_params{};
  const ice::NoGradGuard guard(legacy_params);
  std::cout << "inside guard(legacy_params), forward(x).source().has_value() = "
            << model.forward(x).source().has_value() << "\n";

  return 0;
}
```

Output

```text
inside guard(legacy_params), forward(x).source().has_value() = 0
```

### Example 3

`Model::infer` runs through `Coach`, which wraps the whole pass in its
own `NoGradGuard` -- the returned predictions come back already
untracked, with no extra code at the call site. A direct `forward`
call made outside any guard, right after, still records normally,
confirming the guard's effect does not leak past `infer`'s own scope.

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
  options.execution.max_epochs = 100;
  options.learning_rate = 0.1;
  options.validation.split_ratio = 0.0;
  model.learn(x, y, options);

  const pluss::Series one_row = pluss::make_series({1.0f, 2.0f}, {1, 2});

  const pluss::Series inferred = model.infer(one_row);
  std::cout << "infer(one_row).source().has_value() = " << inferred.source().has_value() << "\n";

  const pluss::Series forwarded = model.forward(one_row);
  std::cout << "forward(one_row).source().has_value() = " << forwarded.source().has_value() << "\n";

  return 0;
}
```

Output

```text
infer(one_row).source().has_value() = 0
forward(one_row).source().has_value() = 1
```

## Changelog

- **0.1** — the `ice`-local no-grad guard, delegating to
  `nn::NoGradGuard`; wired into `Coach::learn`'s validation pass,
  `Coach::assess`, and `Coach::infer`.

## See Also

- [`nn::NoGradGuard`](../../nn/grad/NoGradGuard.md) -- the type this
  delegates to, with the full mechanics (`AutoGradMode`, nesting,
  restore-on-exit).
- [`Coach`](../coach/Coach.md) -- where this guard is actually
  constructed on behalf of `Model::learn`/`infer`/`assess`.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
