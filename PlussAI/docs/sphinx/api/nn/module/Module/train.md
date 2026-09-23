<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e5506e3bf0b4 -->

# `pluss::ai::nn::Module::train`

[Back to `pluss::ai::nn::Module`](../Module.md)

## Signature

```cpp
virtual void nn::Module::train(bool enabled = true) noexcept;
```

## Parameters

`bool enabled`
: The mode to enter. `true` (the default) enters training mode; `false`
  enters evaluation mode -- equivalent to calling [`eval`](eval.md).

## Returns

`void`. Mutates `is_training()` on this module and, recursively, on every
module registered as a child (directly or transitively, through
`module_record`). There is no return value to check -- `train` cannot fail.

## Errors

None. `train` is `noexcept` and has no preconditions.

## Practical Examples

Three examples: mode propagation through a composed network, a bare leaf
layer with nothing to propagate to, and confirmation that mode is scoped to
the registration tree rather than being global.

### Example 1

A two-layer `Schedule` starts in training mode (the default every `Module`
constructs into). Calling `train(false)` on the `Schedule` flips its own
`is_training()` and both children's, in one call -- you never need to walk
`named_modules()` yourself to propagate mode.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Schedule net;
  net.then("a", nn::Fusion(3, 4));
  net.then("b", nn::Fusion(4, 2));

  std::cout << "before: net.is_training()=" << std::boolalpha << net.is_training() << "\n";

  net.train(false);
  std::cout << "after train(false): net.is_training()=" << net.is_training() << "\n";
  for (auto& named : net.named_modules()) {
    if (named.name.empty()) continue;
    std::cout << "  child '" << named.name << "'.is_training()=" << named.module->is_training() << "\n";
  }

  net.eval();
  std::cout << "after eval(): net.is_training()=" << net.is_training() << "\n";

  net.train();
  std::cout << "after train(): net.is_training()=" << net.is_training() << "\n";

  return 0;
}
```

Output

```text
before: net.is_training()=true
after train(false): net.is_training()=false
  child 'a'.is_training()=false
  child 'b'.is_training()=false
after eval(): net.is_training()=false
after train(): net.is_training()=true
```

### Example 2

A leaf layer with no registered children -- `train` still works exactly the
same way, there is simply nothing below it to propagate to.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Fusion leaf(3, 2);  // no children registered
  std::cout << "leaf.is_training()=" << std::boolalpha << leaf.is_training() << "\n";
  leaf.train(false);
  std::cout << "after leaf.train(false): " << leaf.is_training() << "\n";
  leaf.train();
  std::cout << "after leaf.train(): " << leaf.is_training() << "\n";

  return 0;
}
```

Output

```text
leaf.is_training()=true
after leaf.train(false): false
after leaf.train(): true
```

### Example 3

Two independent `Fusion` layers, neither registered as the other's child.
Calling `train` on one never affects the other -- propagation follows the
registration tree built by `module_record`, not global state.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Fusion a(3, 2);
  nn::Fusion b(3, 2);  // independent -- never registered as a's child

  a.train(false);
  std::cout << "a.is_training()=" << std::boolalpha << a.is_training() << "\n";
  std::cout << "b.is_training()=" << b.is_training() << " (unaffected -- propagation only follows registration)\n";

  return 0;
}
```

Output

```text
a.is_training()=false
b.is_training()=true (unaffected -- propagation only follows registration)
```

## See Also

- [`eval`](eval.md) -- `train(false)` by another name.
- [`snapshot`](snapshot.md) -- captures the current mode alongside parameter
  and buffer values.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to.html">
    <span class="label">Previous</span>
    <span class="title">to</span>
  </a>
</div>
<!-- apiref:nav:end -->
