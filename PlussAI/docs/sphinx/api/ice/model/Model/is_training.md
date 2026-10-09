<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 2c7e03a76a51 -->
# `pluss::ai::ice::Model::is_training`

[Back to `pluss::ai::ice::Model`](../Model.md)

## Signature

```cpp
bool pluss::ai::ice::Model::is_training() const noexcept;
```

## What It Does

Reports whether the model is in training mode ([`train`](train.md)) or
evaluation mode ([`eval`](eval.md)/`train(false)`). `true` by default on
a freshly constructed model, matching every registered `nn::Module`'s
own default. [`Coach::learn`](../../coach/Coach.md) sets this around the
training/validation passes it runs; [`infer`](infer.md)/[`assess`](assess.md)
set it to `false` for their own duration.

## Parameters

None.

## Returns

`bool`.

## Errors

None. `is_training` is `noexcept` and never throws.

## Practical Examples

Three examples: the default, after `eval()`, and after `train()`.

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
  std::cout << "is_training() default = " << model.is_training() << "\n";

  return 0;
}
```

Output

```text
is_training() default = 1
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
  model.eval();
  std::cout << "is_training() after eval() = " << model.is_training() << "\n";

  return 0;
}
```

Output

```text
is_training() after eval() = 0
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
  std::cout << "is_training() after train() = " << model.is_training() << "\n";

  return 0;
}
```

Output

```text
is_training() after train() = 1
```

## See Also

- [`train`](train.md) / [`eval`](eval.md) -- set this flag.
- [`ready`](ready.md) -- a different, independent flag (whether `learn`/
  `load` has run at all).

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="infer_step.html">
    <span class="label">Previous</span>
    <span class="title">infer_step</span>
  </a>
  <a class="next-link" href="learn.html">
    <span class="label">Next</span>
    <span class="title">learn</span>
  </a>
</div>
<!-- apiref:nav:end -->
