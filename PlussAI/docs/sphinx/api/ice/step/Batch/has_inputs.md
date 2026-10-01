<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 5440f801ebdd -->
# `pluss::ai::ice::Batch::has_inputs`

[Back to `pluss::ai::ice::Batch`](../Batch.md)

## Signature

```cpp
bool pluss::ai::ice::Batch::has_inputs() const noexcept;
```

## What It Does

Equivalent to `has("inputs")` -- `inputs` is just the field named
`"inputs"`, read through a typed convenience accessor rather than
`field("inputs")` by hand.

## Parameters

None.

## Returns

`bool` -- `true` if [`set_inputs`](set_inputs.md) (or `set("inputs",
...)` directly) has been called.

## Errors

None. `has_inputs` is `noexcept` and never throws.

## Practical Examples

One example: before and after `set_inputs`.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  ice::Batch batch{};
  std::cout << "has_inputs() before set = " << batch.has_inputs() << "\n";

  batch.set_inputs(pluss::make_series({1.0f, 2.0f, 3.0f}, {3}));
  std::cout << "has_inputs() after set = " << batch.has_inputs() << "\n";

  return 0;
}
```

Output

```text
has_inputs() before set = 0
has_inputs() after set = 1
```

### Example 2

`set("inputs", ...)` directly counts the same as `set_inputs` -- there
is nothing special about `set_inputs` beyond being a named wrapper.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  ice::Batch batch{};
  batch.set("inputs", pluss::make_series({1.0f}, {1}));

  std::cout << "has_inputs() = " << batch.has_inputs() << "\n";

  return 0;
}
```

Output

```text
has_inputs() = 1
```

### Example 3

`has_inputs` is independent of `has_targets` -- setting one does not set
the other.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  ice::Batch batch{};
  batch.set_targets(pluss::make_series({1.0f}, {1}));

  std::cout << "has_inputs() = " << batch.has_inputs() << "\n";
  std::cout << "has_targets() = " << batch.has_targets() << "\n";

  return 0;
}
```

Output

```text
has_inputs() = 0
has_targets() = 1
```

## See Also

- [`inputs`](inputs.md) / [`set_inputs`](set_inputs.md) -- read/write
  the same field.
- [`has`](has.md) -- the general form, for any field name.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="has.html">
    <span class="label">Previous</span>
    <span class="title">has</span>
  </a>
  <a class="next-link" href="has_targets.html">
    <span class="label">Next</span>
    <span class="title">has_targets</span>
  </a>
</div>
<!-- apiref:nav:end -->
