<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a25e8ea39c48 -->
# `pluss::ai::ice::Batch::has_targets`

[Back to `pluss::ai::ice::Batch`](../Batch.md)

## Signature

```cpp
bool pluss::ai::ice::Batch::has_targets() const noexcept;
```

## What It Does

Reports whether [`set_targets`](set_targets.md) has been called --
equivalently, whether [`targets()`](targets.md) is `defined()`. Unlike
every other field, `targets` is tracked as its own dedicated member
rather than living in the plain [`fields()`](fields.md) list, so this is
not `has("targets")` (which would check an entirely separate,
never-set-by-`set_targets` field of that literal name).

## Parameters

None.

## Returns

`bool` -- `true` if `set_targets` has been called.

## Errors

None. `has_targets` is `noexcept` and never throws.

## Practical Examples

One example: before and after `set_targets`.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  ice::Batch batch{};
  std::cout << "has_targets() before set = " << batch.has_targets() << "\n";

  batch.set_targets(pluss::make_series({0.0f, 1.0f}, {2}));
  std::cout << "has_targets() after set = " << batch.has_targets() << "\n";

  return 0;
}
```

Output

```text
has_targets() before set = 0
has_targets() after set = 1
```

### Example 2

`has_targets` is independent of `has_inputs` -- setting one does not set
the other.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  ice::Batch batch{};
  batch.set_inputs(pluss::make_series({1.0f}, {1}));

  std::cout << "has_targets() = " << batch.has_targets() << "\n";
  std::cout << "has_inputs() = " << batch.has_inputs() << "\n";

  return 0;
}
```

Output

```text
has_targets() = 0
has_inputs() = 1
```

### Example 3

`targets` is never counted in [`fields()`](fields.md), even though
`has_targets` reports it as set -- the two live in separate storage.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;

  ice::Batch batch{};
  batch.set_targets(pluss::make_series({1.0f}, {1}));

  std::cout << "fields().size() = " << batch.fields().size() << "\n";
  std::cout << "has_targets() = " << batch.has_targets() << "\n";

  return 0;
}
```

Output

```text
fields().size() = 0
has_targets() = 1
```

## See Also

- [`targets`](targets.md) / [`set_targets`](set_targets.md) -- read/write
  this field.
- [`has`](has.md) -- the general form, for any other field name.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="has_inputs.html">
    <span class="label">Previous</span>
    <span class="title">has_inputs</span>
  </a>
  <a class="next-link" href="inputs.html">
    <span class="label">Next</span>
    <span class="title">inputs</span>
  </a>
</div>
<!-- apiref:nav:end -->
