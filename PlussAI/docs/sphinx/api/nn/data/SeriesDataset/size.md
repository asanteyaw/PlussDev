<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e5408de20365 -->

# `pluss::loader::SeriesDataset::size`

[Back to `pluss::loader::SeriesDataset`](../SeriesDataset.md)

## Signature

```cpp
std::int64_t pluss::loader::SeriesDataset::size() const override;
```

## What It Does

Returns the sample count every field shares along
[`batch_dim()`](batch_dim.md) -- computed once, at construction, and
validated to be identical across every field. Overrides
[`Dataset::size`](../Dataset/size.md).

## Parameters

None.

## Returns

`std::int64_t` -- the shared sample count.

## Errors

None from `size()` itself -- the shared-size invariant it reports is
enforced at construction, not on each call.

## Practical Examples

Three examples: the default `batch_dim = 0` case, a non-default
`batch_dim`, and confirming construction rejects fields that disagree on
sample count.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4, 1});
  const pluss::Series y = pluss::make_series({0.0, 1.0, 1.0, 0.0}, {4});
  loader::SeriesDataset dataset({x, y});

  std::cout << "size() = " << dataset.size() << "\n";

  return 0;
}
```

Output

```text
size() = 4
```

### Example 2

Samples along axis 1 of a 2-feature-by-4-sample matrix -- `size()` reports
`4`, the shared count along `batch_dim = 1`.

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  const pluss::Series data = pluss::make_series({1.0, 2.0, 3.0, 4.0, 10.0, 20.0, 30.0, 40.0}, {2, 4});
  loader::SeriesDataset dataset({data}, /*batch_dim=*/1);

  std::cout << "size() = " << dataset.size() << ", batch_dim() = " << dataset.batch_dim() << "\n";

  return 0;
}
```

Output

```text
size() = 4, batch_dim() = 1
```

### Example 3

Two fields disagreeing on their shared-axis sample count throw at
construction -- `size()` is only ever computed once that invariant
already holds.

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>
#include <string>

int main() {
  namespace loader = pluss::loader;

  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4, 1});
  const pluss::Series y_wrong_size = pluss::make_series({0.0, 1.0, 1.0}, {3});

  try {
    loader::SeriesDataset dataset({x, y_wrong_size});
  } catch (const std::exception& e) {
    const std::string message(e.what());
    std::cout << "caught: " << message.substr(0, message.find('\n')) << "\n";
  }

  return 0;
}
```

Output

```text
caught: pluss::SeriesDataset: all fields must share identical sample count on batch_dim
```

## See Also

- [`batch_dim`](batch_dim.md) -- which axis `size()` counts along.
- [`fields`](fields.md) / [`materialize_fields`](materialize_fields.md) --
  the fields `size()` was validated against.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="materialize_fields.html">
    <span class="label">Previous</span>
    <span class="title">materialize_fields</span>
  </a>
</div>
<!-- apiref:nav:end -->
