<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 4bcf4854c877 -->

# `pluss::ai::nn::Module::eval`

[Back to `pluss::ai::nn::Module`](../Module.md)

## Signature

```cpp
void nn::Module::eval() noexcept;
```

## Parameters

None.

## Returns

`void`. Exactly equivalent to `train(false)` -- see [`train`](train.md) for
the full mutation and propagation behavior.

## Errors

None.

## Practical Examples

Three examples: on a composed network (reusing [`train`](train.md)'s), on a
bare leaf layer, and confirming a repeated call is always safe.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Schedule net;
  net.then("a", nn::Fusion(3, 4));
  net.then("b", nn::Fusion(4, 2));

  net.train(false);
  net.eval();
  std::cout << "after eval(): net.is_training()=" << std::boolalpha << net.is_training() << "\n";

  return 0;
}
```

Output

```text
after eval(): net.is_training()=false
```

### Example 2

A leaf layer with no children.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Fusion leaf(3, 2);
  leaf.eval();
  std::cout << "leaf.is_training() after eval()=" << std::boolalpha << leaf.is_training() << "\n";

  return 0;
}
```

Output

```text
leaf.is_training() after eval()=false
```

### Example 3

Calling `eval()` a second time is always safe -- there is no state that
makes a repeated call meaningfully different from the first.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Fusion leaf(3, 2);
  leaf.eval();
  leaf.eval();  // idempotent -- calling it again is always safe
  std::cout << "leaf.is_training() after two eval() calls=" << std::boolalpha << leaf.is_training() << "\n";

  return 0;
}
```

Output

```text
leaf.is_training() after two eval() calls=false
```

## See Also

- [`train`](train.md) -- `eval()` is exactly `train(false)`.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="buffers.html">
    <span class="label">Previous</span>
    <span class="title">buffers</span>
  </a>
  <a class="next-link" href="modules.html">
    <span class="label">Next</span>
    <span class="title">modules</span>
  </a>
</div>
<!-- apiref:nav:end -->
