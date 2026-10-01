<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 254b184aad8e -->

# `pluss::loader::DataBatch::empty`

[Back to `pluss::loader::DataBatch`](../DataBatch.md)

## Signature

```cpp
bool pluss::loader::DataBatch::empty() const noexcept;
```

## What It Does

Reports whether this batch holds any fields at all -- `fields.empty()`.
`DataLoader`/`Dataset::get` never produce an empty batch in ordinary use;
this is mainly relevant for a default-constructed `DataBatch` before
fields are pushed onto it.

## Parameters

None.

## Returns

`bool` -- `true` when `size() == 0`.

## Errors

None. `empty` is `noexcept` and has no preconditions.

## Practical Examples

Three examples: a default-constructed batch, a batch with fields pushed
onto it, and confirming `empty()` flips the moment the first field is
added.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  loader::DataBatch batch;
  std::cout << "empty() = " << std::boolalpha << batch.empty() << "\n";

  return 0;
}
```

Output

```text
empty() = true
```

### Example 2

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  loader::DataBatch batch;
  batch.fields.push_back(pluss::make_series({1.0, 2.0}, {2}));

  std::cout << "empty() = " << std::boolalpha << batch.empty() << "\n";

  return 0;
}
```

Output

```text
empty() = false
```

### Example 3

`empty()` reflects `fields` directly, live -- pushing then popping a
field flips it back.

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  loader::DataBatch batch;
  std::cout << "before push: empty() = " << std::boolalpha << batch.empty() << "\n";

  batch.fields.push_back(pluss::make_series({1.0}, {1}));
  std::cout << "after push: empty() = " << batch.empty() << "\n";

  batch.fields.pop_back();
  std::cout << "after pop: empty() = " << batch.empty() << "\n";

  return 0;
}
```

Output

```text
before push: empty() = true
after push: empty() = false
after pop: empty() = true
```

## See Also

- [`size`](size.md) -- `size() == 0` is exactly `empty()`.
- [`x`](x.md) -- throws on an empty batch.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="at.html">
    <span class="label">Previous</span>
    <span class="title">at</span>
  </a>
  <a class="next-link" href="has_y.html">
    <span class="label">Next</span>
    <span class="title">has_y</span>
  </a>
</div>
<!-- apiref:nav:end -->
