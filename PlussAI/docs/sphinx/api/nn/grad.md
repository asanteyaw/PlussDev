# `pluss::ai::nn::grad`

`grad` owns the one global switch that decides whether new operations
build a reverse-mode differentiation graph at all, and the two RAII guards
built around it. Every other category in this reference (`module`,
`optim`, every layer family) reads or is read by this switch indirectly --
whether a `Module`'s parameters accumulate a graph on `forward`, whether an
optimizer's `step()` has a sensitivity to consume -- but the switch itself,
and the safe way to flip it for a scope, lives here.

## The Surface

| Entity | What it gives you |
| --- | --- |
| [`GradMode`](grad/GradMode.md) | the global switch itself -- `is_enabled()` / `set_enabled(bool)` |
| [`AutoGradMode`](grad/AutoGradMode.md) | an RAII guard that sets `GradMode` to a fixed value and restores the previous value on scope exit |
| [`NoGradGuard`](grad/NoGradGuard.md) | the common case: an RAII guard that disables recording for its scope, built directly on `AutoGradMode` |

## How To Call It

Turn gradient recording off by hand, read the state, and restore it. Then do the same with the scoped guard. The program prints the recording state at each point.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/nn.h>
#include <pluss/nn.h>

#include <iostream>
#include <tuple>
#include <vector>

int main() {
  namespace nn = pluss::ai::nn;
  const bool previous = nn::GradMode::is_enabled();
  nn::GradMode::set_enabled(false);
  const bool inside = nn::GradMode::is_enabled();
  nn::GradMode::set_enabled(previous);
  std::cout << "grad enabled inside = " << (inside ? "true" : "false") << "\n";
  {
    const nn::NoGradGuard no_grad;
    std::cout << "grad enabled inside guard = " << (nn::GradMode::is_enabled() ? "true" : "false") << "\n";
  }
  std::cout << "grad restored = " << (nn::GradMode::is_enabled() == previous ? "true" : "false") << "\n";
  return 0;
}
```

Output

```text
grad enabled inside = false
grad enabled inside guard = false
grad restored = true
```

`GradMode` is a plain global switch with no owned state of its own to
construct or destroy -- both its methods are `static`. `AutoGradMode` and
`NoGradGuard` are the RAII wrapper around it: construct one to change the
switch, let it go out of scope to restore what was there before.

## Related Pages

- [`module`](module.md) -- `GradMode` governs whether ops on a `Module`'s
  registered parameters build a graph on `forward`.
- [`optim`](optim.md) -- an optimizer's `step()` consumes whatever
  sensitivity a graph recorded while `GradMode` was enabled; a parameter
  updated under `NoGradGuard` has nothing for `step()` to read.
- [`data`](data.md) -- `pluss::tils::DataLoader`'s own batch construction
  wraps itself in `NoGradGuard`, since selecting/collating a batch is
  bookkeeping that must never itself become part of a training graph.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

grad/Introduction
grad/GradMode
grad/AutoGradMode
grad/NoGradGuard
```
<!-- apiref:toctree:end -->
