<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 386062da3539 -->

# `pluss::loader::DataBatch::size`

[Back to `pluss::loader::DataBatch`](../DataBatch.md)

## Signature

```cpp
std::size_t pluss::loader::DataBatch::size() const noexcept;
```

## What It Does

Returns how many fields this batch holds -- `fields.size()`, provided
directly. Not the number of *samples* in the batch (every field's own
`Series` shape carries that); this is the field *count*, e.g. `2` for a
typical `(x, y)` batch.

## Parameters

None.

## Returns

`std::size_t` -- the field count.

## Errors

None. `size` is `noexcept` and has no preconditions.

## Practical Examples

Three examples: a three-field batch, a default-constructed (empty)
batch, and `size()` on a batch actually produced by `DataLoader`
iteration.

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
  batch.fields.push_back(pluss::make_series({9.0, 9.0}, {2}));

  std::cout << "size() = " << batch.size() << "\n";

  return 0;
}
```

Output

```text
size() = 3
```

### Example 2

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  loader::DataBatch empty_batch;
  std::cout << "size() = " << empty_batch.size() << "\n";

  return 0;
}
```

Output

```text
size() = 0
```

### Example 3

A batch iterated from a real `DataLoader` over two fields (`x`, `y`) --
`size()` reports `2`, matching the field count the loader was
constructed with, regardless of `batch_size`.

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4, 1});
  const pluss::Series y = pluss::make_series({0.0, 1.0, 1.0, 0.0}, {4});

  loader::DataLoaderOptions options{};
  options.batch_size = 2;
  loader::DataLoader data_loader({x, y}, options);

  for (auto batch : data_loader) {
    std::cout << "batch.size() = " << batch.size() << "\n";
  }

  return 0;
}
```

Output

```text
batch.size() = 2
batch.size() = 2
```

## See Also

- [`empty`](empty.md) -- `size() == 0`.
- [`at`](at.md) -- valid indices are `[0, size())`.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="has_y.html">
    <span class="label">Previous</span>
    <span class="title">has_y</span>
  </a>
  <a class="next-link" href="x.html">
    <span class="label">Next</span>
    <span class="title">x</span>
  </a>
</div>
<!-- apiref:nav:end -->
