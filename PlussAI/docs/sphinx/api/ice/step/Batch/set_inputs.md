<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 9b7f5990cd6f -->
# `pluss::ai::ice::Batch::set_inputs`

[Back to `pluss::ai::ice::Batch`](../Batch.md)

## Signature

```cpp
void pluss::ai::ice::Batch::set_inputs(pluss::Series values);
```

## What It Does

Equivalent to `set("inputs", values)` -- the typed convenience wrapper
for the one field every `Model`'s default `forward(const Batch&)`
override reads.

## Parameters

`Series values`
: The input tensor.

## Returns

`void`.

## Errors

None from `set_inputs` itself.

## Practical Examples

One example: setting `inputs`, then confirming `has_inputs`/`inputs`
both reflect it.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  ice::Batch batch{};
  batch.set_inputs(pluss::make_series({1.0f, 2.0f, 3.0f}, {3}));

  std::cout << "has_inputs() = " << batch.has_inputs() << "\n";
  std::cout << "inputs():\n" << batch.inputs() << "\n";

  return 0;
}
```

Output

```text
has_inputs() = 1
inputs():
pluss::Series(shape=[3], strides=[1], dtype=float32, platform=cpu, numel=3, contiguous=true)
data=[1, 2, 3]
```

### Example 2

Calling `set_inputs` a second time overwrites the first value, matching
[`set`](set.md)'s own replace-in-place behavior.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  ice::Batch batch{};
  batch.set_inputs(pluss::make_series({1.0f}, {1}));
  batch.set_inputs(pluss::make_series({9.0f}, {1}));

  std::cout << "inputs():\n" << batch.inputs() << "\n";

  return 0;
}
```

Output

```text
inputs():
pluss::Series(shape=[1], strides=[1], dtype=float32, platform=cpu, numel=1, contiguous=true)
data=[9]
```

### Example 3

Equivalent to `set("inputs", values)` directly -- both spellings produce
an identical field.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  ice::Batch via_set_inputs{};
  via_set_inputs.set_inputs(pluss::make_series({1.0f, 2.0f}, {2}));

  ice::Batch via_set{};
  via_set.set("inputs", pluss::make_series({1.0f, 2.0f}, {2}));

  const double diff = via_set_inputs.inputs().sub(via_set.inputs()).abs().amax().item<double>();
  std::cout << "max diff = " << diff << "\n";

  return 0;
}
```

Output

```text
max diff = 0
```

## See Also

- [`has_inputs`](has_inputs.md) / [`inputs`](inputs.md) -- read this
  field back.
- [`set`](set.md) -- the general form this delegates to.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="set.html">
    <span class="label">Previous</span>
    <span class="title">set</span>
  </a>
  <a class="next-link" href="set_targets.html">
    <span class="label">Next</span>
    <span class="title">set_targets</span>
  </a>
</div>
<!-- apiref:nav:end -->
