<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 60b9a44c73be -->

# `pluss::loader::SeriesDataset::fields`

[Back to `pluss::loader::SeriesDataset`](../SeriesDataset.md)

## Signature

```cpp
const std::vector<pluss::Series>& pluss::loader::SeriesDataset::fields() const noexcept;
```

## What It Does

Returns the exact fields this dataset was constructed with, unchanged --
the same values [`materialize_fields()`](materialize_fields.md) returns
(a copy of the vector, but the same underlying `Series` values), provided
directly so a caller does not need to go through the `Dataset`-interface
spelling to read them back.

## Parameters

None.

## Returns

`const std::vector<Series>&` -- a reference into the dataset's own
storage, valid for the dataset's lifetime.

## Errors

None. `fields` is `noexcept` and has no preconditions.

## Practical Examples

Three examples: reading fields back after construction, confirming the
count matches what was passed in, and confirming `fields()` and
`materialize_fields()` agree.

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

  std::cout << "fields().size() = " << dataset.fields().size() << "\n";
  std::cout << "fields()[1] = " << dataset.fields()[1] << "\n";

  return 0;
}
```

Output

```text
fields().size() = 2
fields()[1] = pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0, 1, 1, 0]
```

### Example 2

A single-field dataset -- `fields().size()` always matches the number of
`Series` the constructor was given, regardless of how many samples each
one holds.

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0}, {3, 1});
  loader::SeriesDataset dataset({x});

  std::cout << "fields().size() = " << dataset.fields().size() << "\n";

  return 0;
}
```

Output

```text
fields().size() = 1
```

### Example 3

`fields()` and `materialize_fields()` return the same values -- the
latter exists to satisfy `Dataset`'s own interface, the former as a
`noexcept`, reference-returning convenience specific to `SeriesDataset`.

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4, 1});
  loader::SeriesDataset dataset({x});

  const auto materialized = dataset.materialize_fields();
  std::cout << "fields().size() == materialize_fields().size(): "
             << std::boolalpha << (dataset.fields().size() == materialized.size()) << "\n";

  return 0;
}
```

Output

```text
fields().size() == materialize_fields().size(): true
```

## See Also

- [`materialize_fields`](materialize_fields.md) -- the `Dataset`-interface
  spelling of the same fields.
- [`SeriesDataset`](../SeriesDataset.md) -- construction validates every
  field before `fields()` can be called.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="batch_dim.html">
    <span class="label">Previous</span>
    <span class="title">batch_dim</span>
  </a>
  <a class="next-link" href="get.html">
    <span class="label">Next</span>
    <span class="title">get</span>
  </a>
</div>
<!-- apiref:nav:end -->
