<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 028642b2af36 -->
# `pluss::ai::ice::Model::train`

[Back to `pluss::ai::ice::Model`](../Model.md)

## Signature

```cpp
void pluss::ai::ice::Model::train(bool enabled = true) noexcept;
```

## What It Does

Sets training mode on every registered parameter and module --
`train(true)` (the default argument) is training mode,
`train(false)` is evaluation mode (equivalent to calling
[`eval`](eval.md)).

## Parameters

`bool enabled = true`
: `true` for training mode, `false` for evaluation mode.

## Returns

`void`.

## Errors

None. `train` is `noexcept` and never throws.

## Practical Examples

Three examples: the default argument, explicit `false`, and toggling
back and forth.

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
  model.train();
  std::cout << "is_training() after train() = " << model.is_training() << "\n";

  return 0;
}
```

Output

```text
is_training() after train() = 1
```

### Example 2

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;
  namespace nn = pluss::nn;

  ice::Outline model(nn::Fusion(2, 1));
  model.train(false);
  std::cout << "is_training() after train(false) = " << model.is_training() << "\n";

  return 0;
}
```

Output

```text
is_training() after train(false) = 0
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
  model.train(true);
  std::cout << "[1] is_training() = " << model.is_training() << "\n";
  model.train(false);
  std::cout << "[2] is_training() = " << model.is_training() << "\n";
  model.train(true);
  std::cout << "[3] is_training() = " << model.is_training() << "\n";

  return 0;
}
```

Output

```text
[1] is_training() = 1
[2] is_training() = 0
[3] is_training() = 1
```

## See Also

- [`eval`](eval.md) -- equivalent to `train(false)`.
- [`is_training`](is_training.md) -- reads this flag back.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to.html">
    <span class="label">Previous</span>
    <span class="title">to</span>
  </a>
  <a class="next-link" href="training_report.html">
    <span class="label">Next</span>
    <span class="title">training_report</span>
  </a>
</div>
<!-- apiref:nav:end -->
