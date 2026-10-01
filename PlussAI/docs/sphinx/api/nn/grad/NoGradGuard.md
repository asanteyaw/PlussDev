# `pluss::ai::nn::NoGradGuard`

An RAII guard that disables grad recording for its scope -- the common case
built directly on [`AutoGradMode`](AutoGradMode.md): `NoGradGuard` is
exactly `AutoGradMode` fixed to `false`, spelled out as its own type so the
intent at a call site reads as "don't record a graph here" rather than
"force the switch to some value". Reach for `NoGradGuard` by default;
reach for [`AutoGradMode`](AutoGradMode.md) directly only when you
specifically need to force **re-enabling** recording inside an
already-disabled scope (see that page's Example 2).

`pluss::tils::DataLoader`'s own batch-construction code uses exactly this
guard: selecting and collating a batch out of tracked fields is bookkeeping
that must never itself become part of a training graph, so it wraps that
whole step in a `NoGradGuard` -- see Example 2 below for the same pattern.

## Signature

```cpp
nn::NoGradGuard::NoGradGuard() noexcept = default;
nn::NoGradGuard::~NoGradGuard() = default;

nn::NoGradGuard::NoGradGuard(const NoGradGuard&) = delete;
nn::NoGradGuard& nn::NoGradGuard::operator=(const NoGradGuard&) = delete;
nn::NoGradGuard::NoGradGuard(NoGradGuard&&) = delete;
nn::NoGradGuard& nn::NoGradGuard::operator=(NoGradGuard&&) = delete;
```

## Construction

```cpp
const nn::NoGradGuard no_grad;  // disables recording for this scope
```

Takes no arguments -- unlike `AutoGradMode`, there is no value to choose;
`NoGradGuard` always disables. Construction immediately sets
`GradMode::is_enabled()` to `false` (after capturing whatever value was
active, via its internal `AutoGradMode` member) and the destructor restores
it. Neither copyable nor movable, for the same reason as `AutoGradMode`: it
is meant to guard exactly the scope it is declared in.

## State

None directly on `NoGradGuard` itself -- it holds one private
`AutoGradMode(false)` member, which is where the previous-value bookkeeping
actually lives.

## Methods

None besides the constructor/destructor -- its entire effect is its
lifetime, identically to `AutoGradMode`.

## Practical Examples

Three examples: the basic disable-and-restore, the real batch-construction
pattern this type exists for, and nesting inside an outer `AutoGradMode`
guard to confirm it restores to whatever was actually active, not
unconditionally to `true`.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  std::cout << "before: " << std::boolalpha << nn::GradMode::is_enabled() << "\n";
  {
    const nn::NoGradGuard guard;
    std::cout << "inside: " << nn::GradMode::is_enabled() << "\n";
  }
  std::cout << "after:  " << nn::GradMode::is_enabled() << "\n";

  return 0;
}
```

Output

```text
before: true
inside: false
after:  true
```

### Example 2

A tracked field is indexed to build a batch -- the same shape of call
`pluss::tils::DataLoader`'s internal batch construction makes on every
field of every batch. Wrapping the call in `NoGradGuard` keeps the
selection itself out of the training graph, even though the field being
read from tracks its slope; the identical call made outside the guard
records normally.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>
#include <vector>

int main() {
  namespace nn = pluss::ai::nn;
  using pluss::Config;
  using pluss::ScalarType;
  using pluss::Series;

  const Config f32 = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  Series x = pluss::make_series(std::vector<float>{10.0F, 20.0F, 30.0F, 40.0F}, {4}, f32);
  x.tracks_slope_(true);

  const std::vector<std::int64_t> ids{0, 2};
  const Config idx_cfg = Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU);
  const Series idx = pluss::make_series(ids, {2}, idx_cfg);

  Series batch = pluss::series::undefined();
  {
    const nn::NoGradGuard no_grad;
    batch = x.index_select(0, idx);
  }
  std::cout << "batch.tracks_slope() = " << std::boolalpha << batch.tracks_slope() << " (bookkeeping, not recorded)\n";

  const Series tracked_select = x.index_select(0, idx);
  std::cout << "same call without the guard: tracks_slope() = " << tracked_select.tracks_slope() << "\n";

  return 0;
}
```

Output

```text
batch.tracks_slope() = false (bookkeeping, not recorded)
same call without the guard: tracks_slope() = true
```

### Example 3

Nested inside an outer `AutoGradMode(true)`, `NoGradGuard` still disables
locally, and on exit restores to whatever was active right before it was
constructed -- the outer guard's `true` -- matching `AutoGradMode`'s own
restore contract exactly, since `NoGradGuard` is built on it.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const nn::AutoGradMode outer(true);
  std::cout << "outer enabled:             " << std::boolalpha << nn::GradMode::is_enabled() << "\n";
  {
    const nn::NoGradGuard guard;
    std::cout << "nested NoGradGuard:        " << nn::GradMode::is_enabled() << "\n";
  }
  std::cout << "after nested guard exits: " << nn::GradMode::is_enabled() << "\n";

  return 0;
}
```

Output

```text
outer enabled:             true
nested NoGradGuard:        false
after nested guard exits: true
```

## Changelog

- **0.1** — the disable-for-this-scope RAII guard, built on `AutoGradMode`.

## See Also

- [`AutoGradMode`](AutoGradMode.md) -- the general save/restore guard this
  type is built on.
- [`GradMode`](GradMode.md) -- the underlying switch.
- [`data`](../data.md) -- `pluss::tils::DataLoader`'s batch construction
  wraps itself in exactly this guard.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
