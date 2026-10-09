<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a4414cefbe05 -->
# `pluss::ai::ice::Model::eval`

[Back to `pluss::ai::ice::Model`](../Model.md)

## Signature

```cpp
void pluss::ai::ice::Model::eval() noexcept;
```

## What It Does

Equivalent to `train(false)` -- switches every registered parameter and
module to evaluation mode.

## Parameters

None.

## Returns

`void`.

## Errors

None. `eval` is `noexcept` and never throws.

## Practical Examples

Three examples: calling `eval`, confirming it matches `train(false)`
exactly, and toggling back with `train()`.

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
  model.eval();
  std::cout << "is_training() after eval() = " << model.is_training() << "\n";

  return 0;
}
```

Output

```text
is_training() after eval() = 0
```

### Example 2

`eval()` and `train(false)` produce the identical state.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;
  namespace nn = pluss::nn;

  ice::Outline via_eval(nn::Fusion(2, 1));
  via_eval.eval();

  ice::Outline via_train_false(nn::Fusion(2, 1));
  via_train_false.train(false);

  std::cout << "via_eval.is_training() = " << via_eval.is_training() << "\n";
  std::cout << "via_train_false.is_training() = " << via_train_false.is_training() << "\n";

  return 0;
}
```

Output

```text
via_eval.is_training() = 0
via_train_false.is_training() = 0
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
  model.eval();
  model.train();
  std::cout << "is_training() after eval() then train() = " << model.is_training() << "\n";

  return 0;
}
```

Output

```text
is_training() after eval() then train() = 1
```

## See Also

- [`train`](train.md) -- `eval()` is equivalent to `train(false)`.
- [`is_training`](is_training.md) -- reads this flag back.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="compute_loss.html">
    <span class="label">Previous</span>
    <span class="title">compute_loss</span>
  </a>
  <a class="next-link" href="forward.html">
    <span class="label">Next</span>
    <span class="title">forward</span>
  </a>
</div>
<!-- apiref:nav:end -->
