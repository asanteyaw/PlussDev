<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 46957aa44d90 -->
# `pluss::ai::ice::Batch::inputs`

[Back to `pluss::ai::ice::Batch`](../Batch.md)

## Signature

```cpp
const pluss::Series& pluss::ai::ice::Batch::inputs() const;
pluss::Series& pluss::ai::ice::Batch::inputs();
```

## What It Does

Equivalent to `field("inputs")` -- the typed accessor for the one field
[`Model`](../../model/Model.md)'s default `forward(const Batch&)`
override reads to call `forward(const Series&)`. Throws under the same
condition `field` does: when `set_inputs` (or `set("inputs", ...)`)
hasn't been called.

## Parameters

None.

## Returns

`const Series&` (on a `const Batch`) or `Series&` (otherwise).

## Errors

- No `"inputs"` field has been set -- throws.

## Practical Examples

One example: reading back a set `inputs` field.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  ice::Batch batch{};
  batch.set_inputs(pluss::make_series({1.0f, 2.0f, 3.0f}, {3}));

  std::cout << "inputs():\n" << batch.inputs() << "\n";

  return 0;
}
```

Output

```text
inputs():
pluss::Series(shape=[3], strides=[1], dtype=float32, platform=cpu, numel=3, contiguous=true)
data=[1, 2, 3]
```

### Example 2

The non-`const` overload returns a mutable reference, so `inputs` can be
edited in place.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  ice::Batch batch{};
  batch.set_inputs(pluss::make_series({1.0f, 2.0f}, {2}));
  batch.inputs() = batch.inputs().mul(10.0f);

  std::cout << "inputs():\n" << batch.inputs() << "\n";

  return 0;
}
```

Output

```text
inputs():
pluss::Series(shape=[2], strides=[1], dtype=float32, platform=cpu, numel=2, contiguous=true)
data=[10, 20]
```

### Example 3

Calling `inputs()` before `set_inputs` throws -- the same error
`field("inputs")` would.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  ice::Batch batch{};
  try {
    batch.inputs();
  } catch (const std::exception& e) {
    const std::string message(e.what());
    std::cout << "threw: " << message.substr(0, message.find('\n')) << "\n";
  }

  return 0;
}
```

Output

```text
threw: pluss::ai::ice::Batch::field: no field named 'inputs'.
```

## See Also

- [`has_inputs`](has_inputs.md) -- checks presence without throwing.
- [`set_inputs`](set_inputs.md) -- sets this field.
- [`field`](field.md) -- the general form this delegates to.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="has_targets.html">
    <span class="label">Previous</span>
    <span class="title">has_targets</span>
  </a>
  <a class="next-link" href="set.html">
    <span class="label">Next</span>
    <span class="title">set</span>
  </a>
</div>
<!-- apiref:nav:end -->
