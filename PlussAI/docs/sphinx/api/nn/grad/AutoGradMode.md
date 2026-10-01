# `pluss::ai::nn::AutoGradMode`

An RAII guard around [`GradMode`](GradMode.md): its constructor reads the
current thread's grad-recording switch, sets it to a fixed value you
supply, and its destructor restores whatever value was active right before
construction. It is the general save/restore primitive
[`NoGradGuard`](NoGradGuard.md) is built from -- reach for `AutoGradMode`
directly when you need to *force* a specific value (including
re-**enabling** recording inside an already-disabled scope); reach for
`NoGradGuard` instead when the intent is simply "disable recording here",
which is the far more common case.

Because the restore happens in a destructor, it runs on every exit path --
normal return, early return, or an exception propagating through -- unlike
a bare [`GradMode::set_enabled`](GradMode/set_enabled.md) pair, where
anything skipping the second call leaves the switch changed for the rest of
the thread.

## Signature

```cpp
explicit nn::AutoGradMode::AutoGradMode(bool enabled) noexcept;
nn::AutoGradMode::~AutoGradMode();

nn::AutoGradMode::AutoGradMode(const AutoGradMode&) = delete;
nn::AutoGradMode& nn::AutoGradMode::operator=(const AutoGradMode&) = delete;
nn::AutoGradMode::AutoGradMode(AutoGradMode&&) = delete;
nn::AutoGradMode& nn::AutoGradMode::operator=(AutoGradMode&&) = delete;
```

## Construction

```cpp
const nn::AutoGradMode disable(false);  // force-disable for this scope
const nn::AutoGradMode enable(true);    // force-enable, even inside an outer disabled scope
```

Construction immediately calls `GradMode::set_enabled(enabled)`, after first
capturing `GradMode::is_enabled()`'s value at that moment as the value to
restore later. `AutoGradMode` is neither copyable nor movable -- it is
meant to live as a single named local (or unnamed temporary spanning a
block), guarding exactly the scope it is declared in.

## State

One private `bool` capturing the switch's value at construction time, so
that a nested `AutoGradMode` correctly restores to *its* enclosing scope's
value (which may itself be `true` or `false`), not unconditionally to
`true` -- see Example 2.

## Methods

None besides the constructor/destructor -- there is nothing to call on an
`AutoGradMode` instance; its entire effect is its lifetime.

## Practical Examples

Three examples: the basic force-and-restore, nested guards restoring to the
*enclosing* guard's value rather than the thread's original state, and a
realistic evaluation helper that stays correct even across an early return
or an exception.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  std::cout << "before:  " << std::boolalpha << nn::GradMode::is_enabled() << "\n";
  {
    const nn::AutoGradMode disable(false);
    std::cout << "inside:  " << nn::GradMode::is_enabled() << "\n";
  }
  std::cout << "after:   " << nn::GradMode::is_enabled() << "\n";

  return 0;
}
```

Output

```text
before:  true
inside:  false
after:   true
```

### Example 2

An outer guard disables recording; a nested guard temporarily re-enables it
inside that scope. When the nested guard exits, it restores the value that
was active right before *it* was constructed -- the outer guard's `false`
-- not the thread's original pre-outer-guard state.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const nn::AutoGradMode outer(false);
  std::cout << "outer disabled:              " << std::boolalpha << nn::GradMode::is_enabled() << "\n";
  {
    const nn::AutoGradMode inner(true);
    std::cout << "nested re-enable:            " << nn::GradMode::is_enabled() << "\n";
  }
  std::cout << "after inner exits (restored to outer's false): " << nn::GradMode::is_enabled() << "\n";

  return 0;
}
```

Output

```text
outer disabled:              false
nested re-enable:            true
after inner exits (restored to outer's false): false
```

### Example 3

An `evaluate()` helper disables recording for its whole body via
`AutoGradMode`. Because the restore is a destructor, grad mode comes back
correctly after `evaluate()` returns regardless of how it returns -- there
is no path through this function that could leave the switch stuck off, the
way a bare `set_enabled`/`set_enabled(previous)` pair could if something
returned early between the two calls.

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
  nn::Fusion fc(3, 2, /*bias=*/false, f32);
  const Series x = pluss::make_series(std::vector<float>{1.0F, 2.0F, 3.0F}, {1, 3}, f32);

  const auto evaluate = [&](const Series& input) {
    const nn::AutoGradMode disable(false);
    return fc.forward(input);
  };

  const Series out = evaluate(x);
  std::cout << "inside evaluate(): output.tracks_slope() = " << std::boolalpha << out.tracks_slope() << "\n";
  std::cout << "after evaluate() returns, grad mode restored: " << nn::GradMode::is_enabled() << "\n";

  const Series train_out = fc.forward(x);
  std::cout << "ordinary forward outside evaluate(): output.tracks_slope() = " << train_out.tracks_slope() << "\n";

  return 0;
}
```

Output

```text
inside evaluate(): output.tracks_slope() = false
after evaluate() returns, grad mode restored: true
ordinary forward outside evaluate(): output.tracks_slope() = true
```

## Changelog

- **0.1** — the general save/restore RAII guard around `GradMode`.

## See Also

- [`GradMode`](GradMode.md) -- the switch this guard wraps.
- [`NoGradGuard`](NoGradGuard.md) -- the far more common
  disable-for-this-scope case, built directly on this type.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
