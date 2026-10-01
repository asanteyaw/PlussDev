<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 1197c1f3fde1 -->

# `pluss::loader::DataBatch::x`

[Back to `pluss::loader::DataBatch`](../DataBatch.md)

## Signature

```cpp
pluss::Series& pluss::loader::DataBatch::x();
const pluss::Series& pluss::loader::DataBatch::x() const;
```

## What It Does

Returns the first field -- `at(0)`, under the "input" name by convention.
Every batch produced by [`DataLoader`](../DataLoader.md) or
[`Dataset::get`](../Dataset/get.md) has a first field (a `DataLoader`
constructed with no fields at all fails earlier, at its own
construction), so `x()` is the one accessor on this page that never needs
a `has_*()`-style guard the way [`y()`](y.md) does.

## Parameters

None.

## Returns

`Series&` (or `const Series&`) -- a reference into the batch's own
storage.

## Errors

- Throws if the batch has no fields at all (`fields.empty()`) -- never
  the case for a batch that actually came from `DataLoader`/`Dataset::get`,
  only reachable via a manually default-constructed `DataBatch`.

## Practical Examples

Three examples: `x()` on a manually built batch, `x()` throwing on an
empty batch, and `x()` read from a real `DataLoader` iteration.

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

  std::cout << "x() = " << batch.x() << "\n";

  return 0;
}
```

Output

```text
x() = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 2]
```

### Example 2

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>
#include <string>

int main() {
  namespace loader = pluss::loader;

  loader::DataBatch empty_batch;
  try {
    empty_batch.x();
  } catch (const std::exception& e) {
    const std::string message(e.what());
    std::cout << "caught: " << message.substr(0, message.find('\n')) << "\n";
  }

  return 0;
}
```

Output

```text
caught: pluss::DataBatch::x: batch has no fields
```

### Example 3

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
    std::cout << "x() = " << batch.x() << "\n";
  }

  return 0;
}
```

Output

```text
x() = pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[1],
  [2]]
x() = pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[3],
  [4]]
```

## See Also

- [`y`](y.md) -- the second-field counterpart; requires
  [`has_y`](has_y.md) to be `true`.
- [`at`](at.md) -- `x()` is exactly `at(0)`.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="size.html">
    <span class="label">Previous</span>
    <span class="title">size</span>
  </a>
  <a class="next-link" href="y.html">
    <span class="label">Next</span>
    <span class="title">y</span>
  </a>
</div>
<!-- apiref:nav:end -->
