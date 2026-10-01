<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c2f984fe3970 -->
# `pluss::ai::ice::Batch::set_targets`

[Back to `pluss::ai::ice::Batch`](../Batch.md)

## Signature

```cpp
void pluss::ai::ice::Batch::set_targets(pluss::Series values);
```

## What It Does

Sets the `targets` field, stored separately from every other field (see
[`targets`](targets.md)). Calling this again replaces the previous
value.

## Parameters

`Series values`
: The target tensor `compute_loss` will compare predictions against.

## Returns

`void`.

## Errors

None from `set_targets` itself.

## Practical Examples

One example: setting `targets`, then confirming `has_targets`/`targets`
both reflect it.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  ice::Batch batch{};
  batch.set_targets(pluss::make_series({0.0f, 1.0f}, {2}));

  std::cout << "has_targets() = " << batch.has_targets() << "\n";
  std::cout << "targets():\n" << batch.targets() << "\n";

  return 0;
}
```

Output

```text
has_targets() = 1
targets():
pluss::Series(shape=[2], strides=[1], dtype=float32, platform=cpu, numel=2, contiguous=true)
data=[0, 1]
```

### Example 2

Calling `set_targets` a second time overwrites the first value.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  ice::Batch batch{};
  batch.set_targets(pluss::make_series({1.0f}, {1}));
  batch.set_targets(pluss::make_series({9.0f}, {1}));

  std::cout << "targets():\n" << batch.targets() << "\n";

  return 0;
}
```

Output

```text
targets():
pluss::Series(shape=[1], strides=[1], dtype=float32, platform=cpu, numel=1, contiguous=true)
data=[9]
```

### Example 3

`set_targets` is independent of every other field -- setting it does not
disturb `inputs` or any other already-set field.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  ice::Batch batch{};
  batch.set_inputs(pluss::make_series({1.0f, 2.0f}, {2}));
  batch.set_targets(pluss::make_series({3.0f}, {1}));

  std::cout << "inputs():\n" << batch.inputs() << "\n";
  std::cout << "targets():\n" << batch.targets() << "\n";

  return 0;
}
```

Output

```text
inputs():
pluss::Series(shape=[2], strides=[1], dtype=float32, platform=cpu, numel=2, contiguous=true)
data=[1, 2]
targets():
pluss::Series(shape=[1], strides=[1], dtype=float32, platform=cpu, numel=1, contiguous=true)
data=[3]
```

## See Also

- [`has_targets`](has_targets.md) / [`targets`](targets.md) -- read this
  field back.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="set_inputs.html">
    <span class="label">Previous</span>
    <span class="title">set_inputs</span>
  </a>
  <a class="next-link" href="targets.html">
    <span class="label">Next</span>
    <span class="title">targets</span>
  </a>
</div>
<!-- apiref:nav:end -->
