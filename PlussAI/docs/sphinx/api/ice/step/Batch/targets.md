<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 5568a235aa0c -->
# `pluss::ai::ice::Batch::targets`

[Back to `pluss::ai::ice::Batch`](../Batch.md)

## Signature

```cpp
const pluss::Series& pluss::ai::ice::Batch::targets() const noexcept;
pluss::Series& pluss::ai::ice::Batch::targets() noexcept;
```

## What It Does

Returns the `targets` field directly -- unlike [`inputs`](inputs.md)/
[`field`](field.md), this never throws: if `set_targets` was never
called, it returns an undefined `Series` (`defined() == false`) rather
than throwing. Check [`has_targets`](has_targets.md), or the returned
`Series`'s own `defined()`, before relying on the value.
[`Model::compute_loss(predictions, batch, ctx)`](../../model/Model/compute_loss.md)
is the one built-in caller, and it throws its own, more specific error
("`batch.targets must be defined`") when this comes back undefined.

## Parameters

None.

## Returns

`const Series&` (on a `const Batch`) or `Series&` (otherwise) -- the
targets value, possibly undefined.

## Errors

None. `targets` is `noexcept` and never throws, regardless of whether
`set_targets` was called.

## Practical Examples

Two examples: the undefined default, and a set value.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  ice::Batch batch{};
  std::cout << "targets().defined() before set = " << batch.targets().defined() << "\n";

  return 0;
}
```

Output

```text
targets().defined() before set = 0
```

### Example 2

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  ice::Batch batch{};
  batch.set_targets(pluss::make_series({0.0f, 1.0f}, {2}));

  std::cout << "targets():\n" << batch.targets() << "\n";

  return 0;
}
```

Output

```text
targets():
pluss::Series(shape=[2], strides=[1], dtype=float32, platform=cpu, numel=2, contiguous=true)
data=[0, 1]
```

### Example 3

The non-`const` overload returns a mutable reference, so `targets` can
be edited in place.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  ice::Batch batch{};
  batch.set_targets(pluss::make_series({1.0f, 2.0f}, {2}));
  batch.targets() = batch.targets().mul(2.0f);

  std::cout << "targets():\n" << batch.targets() << "\n";

  return 0;
}
```

Output

```text
targets():
pluss::Series(shape=[2], strides=[1], dtype=float32, platform=cpu, numel=2, contiguous=true)
data=[2, 4]
```

## See Also

- [`has_targets`](has_targets.md) -- checks `defined()` without
  constructing the reference by hand.
- [`set_targets`](set_targets.md) -- sets this field.
- [`Model::compute_loss`](../../model/Model/compute_loss.md) -- the
  built-in consumer.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="set_targets.html">
    <span class="label">Previous</span>
    <span class="title">set_targets</span>
  </a>
</div>
<!-- apiref:nav:end -->
