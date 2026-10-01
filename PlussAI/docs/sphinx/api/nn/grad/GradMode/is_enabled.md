<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 78ba43002ed8 -->

# `pluss::ai::nn::GradMode::is_enabled`

[Back to `pluss::ai::nn::GradMode`](../GradMode.md)

## Signature

```cpp
[[nodiscard]] static bool nn::GradMode::is_enabled() noexcept;
```

## What It Does

Reads the current thread's grad-recording switch. Returns `true` when new
operations on a `Series` that tracks its slope are currently allowed to
record a reverse-mode graph node, `false` when they are not. The value
reflects every [`set_enabled`](set_enabled.md) call and every
[`AutoGradMode`](../AutoGradMode.md)/[`NoGradGuard`](../NoGradGuard.md)
guard currently in scope on the calling thread -- it is a plain read, with
no side effect of its own.

## Parameters

None.

## Returns

`bool` -- the current value of the thread-local switch. Defaults to `true`
on a thread that has never called `set_enabled` or constructed a guard.

## Errors

None. `is_enabled` is `noexcept` and has no preconditions.

## Practical Examples

Three examples: the default value on a thread that has touched nothing
else, its response to a direct `set_enabled` call, and reading it from
inside nested guards to show it reports whatever is actually in effect at
the moment of the call, not just the innermost guard's own argument.

### Example 1

A thread that has never called `set_enabled` or constructed a guard reports
`true` -- recording happens by default; a caller opts out for a scope,
never the reverse.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  std::cout << "is_enabled() on a fresh thread = " << std::boolalpha << nn::GradMode::is_enabled() << "\n";

  return 0;
}
```

Output

```text
is_enabled() on a fresh thread = true
```

### Example 2

`is_enabled()` reflects a direct `set_enabled` call immediately -- there is
no delay or batching, the read and the write operate on the same variable.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  std::cout << "before: " << std::boolalpha << nn::GradMode::is_enabled() << "\n";

  nn::GradMode::set_enabled(false);
  std::cout << "after set_enabled(false): " << nn::GradMode::is_enabled() << "\n";

  nn::GradMode::set_enabled(true);
  std::cout << "after set_enabled(true): " << nn::GradMode::is_enabled() << "\n";

  return 0;
}
```

Output

```text
before: true
after set_enabled(false): false
after set_enabled(true): true
```

### Example 3

Nested guards -- `NoGradGuard` disables, a nested `AutoGradMode(true)`
temporarily re-enables inside it, and `is_enabled()` reports the value that
is actually active at each point, not the outermost or innermost guard's
argument in isolation.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  std::cout << "outside any guard: " << std::boolalpha << nn::GradMode::is_enabled() << "\n";
  {
    const nn::NoGradGuard guard;
    std::cout << "inside NoGradGuard: " << nn::GradMode::is_enabled() << "\n";
    {
      const nn::AutoGradMode reenable(true);
      std::cout << "inside nested AutoGradMode(true): " << nn::GradMode::is_enabled() << "\n";
    }
    std::cout << "back inside NoGradGuard, after nested guard exits: " << nn::GradMode::is_enabled() << "\n";
  }
  std::cout << "outside again, after NoGradGuard exits: " << nn::GradMode::is_enabled() << "\n";

  return 0;
}
```

Output

```text
outside any guard: true
inside NoGradGuard: false
inside nested AutoGradMode(true): true
back inside NoGradGuard, after nested guard exits: false
outside again, after NoGradGuard exits: true
```

## See Also

- [`set_enabled`](set_enabled.md) -- the corresponding write.
- [`AutoGradMode`](../AutoGradMode.md) / [`NoGradGuard`](../NoGradGuard.md)
  -- the RAII guards that call `set_enabled` for you and restore it on scope
  exit.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="set_enabled.html">
    <span class="label">Next</span>
    <span class="title">set_enabled</span>
  </a>
</div>
<!-- apiref:nav:end -->
