<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: b2adbba6c6b3 -->

# `pluss::loader::DataBatch::has_y`

[Back to `pluss::loader::DataBatch`](../DataBatch.md)

## Signature

```cpp
bool pluss::loader::DataBatch::has_y() const noexcept;
```

## What It Does

Reports whether this batch has a second field -- `fields.size() >= 2` --
i.e. whether [`y()`](y.md) is safe to call. Reach for this before calling
`y()` on a batch that might come from a single-field (`x`-only) dataset,
rather than catching the exception `y()` would otherwise throw.

## Parameters

None.

## Returns

`bool` -- `true` when `size() >= 2`.

## Errors

None. `has_y` is `noexcept` and has no preconditions.

## Practical Examples

Three examples: a two-field batch, a single-field batch, and an empty
batch.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  loader::DataBatch batch;
  batch.fields.push_back(pluss::make_series({1.0, 2.0}, {2}));
  batch.fields.push_back(pluss::make_series({0.0, 1.0}, {2}));

  std::cout << "has_y() = " << std::boolalpha << batch.has_y() << "\n";

  return 0;
}
```

Output

```text
has_y() = true
```

### Example 2

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  loader::DataBatch one_field;
  one_field.fields.push_back(pluss::make_series({5.0}, {1}));

  std::cout << "has_y() = " << std::boolalpha << one_field.has_y() << "\n";

  return 0;
}
```

Output

```text
has_y() = false
```

### Example 3

An empty batch also reports `false` -- `has_y()` never throws, unlike
`y()` itself.

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  loader::DataBatch empty_batch;
  std::cout << "has_y() = " << std::boolalpha << empty_batch.has_y() << "\n";

  return 0;
}
```

Output

```text
has_y() = false
```

## See Also

- [`y`](y.md) -- throws when `has_y()` is `false`.
- [`x`](x.md) -- has no equivalent guard; a batch always needs at least
  one field for `x()` to be meaningful.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="empty.html">
    <span class="label">Previous</span>
    <span class="title">empty</span>
  </a>
  <a class="next-link" href="size.html">
    <span class="label">Next</span>
    <span class="title">size</span>
  </a>
</div>
<!-- apiref:nav:end -->
