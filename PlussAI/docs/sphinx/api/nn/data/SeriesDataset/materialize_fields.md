<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d5f1e2b3625d -->

# `pluss::loader::SeriesDataset::materialize_fields`

[Back to `pluss::loader::SeriesDataset`](../SeriesDataset.md)

## Signature

```cpp
std::vector<pluss::Series> pluss::loader::SeriesDataset::materialize_fields() const override;
```

## What It Does

Returns the exact fields this dataset was constructed with -- the same
values [`fields()`](fields.md) returns. Overrides
[`Dataset::materialize_fields`](../Dataset/materialize_fields.md);
unlike a custom `Dataset` subclass, this override is never left at the
unimplemented (empty) default, so a `SeriesDataset` is always immediately
usable with [`DataLoader`](../DataLoader.md).

## Parameters

None.

## Returns

`std::vector<Series>` -- never empty for a validly constructed
`SeriesDataset` (construction itself requires at least one field).

## Errors

None. Construction already validated every field.

## Practical Examples

Three examples: reading the materialized fields directly, constructing a
`DataLoader` from them (proving the "always usable" claim concretely),
and confirming the result matches `fields()`.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4, 1});
  loader::SeriesDataset dataset({x});

  const auto materialized = dataset.materialize_fields();
  std::cout << "materialize_fields().size() = " << materialized.size() << "\n";

  return 0;
}
```

Output

```text
materialize_fields().size() = 1
```

### Example 2

Unlike a custom `Dataset` subclass that forgets to override
`materialize_fields()` (see
[`Dataset::materialize_fields`](../Dataset/materialize_fields.md) Example
2), a `SeriesDataset` never throws here -- `DataLoader` construction
succeeds directly.

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4, 1});
  loader::SeriesDataset dataset({x});

  loader::DataLoader data_loader(dataset);
  std::cout << "batch_count() = " << data_loader.batch_count() << "\n";

  return 0;
}
```

Output

```text
batch_count() = 1
```

### Example 3

`materialize_fields()` and `fields()` return the same values.

```cpp
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>

int main() {
  namespace loader = pluss::loader;

  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4, 1});
  const pluss::Series y = pluss::make_series({0.0, 1.0, 1.0, 0.0}, {4});
  loader::SeriesDataset dataset({x, y});

  std::cout << "materialize_fields().size() == fields().size(): "
             << std::boolalpha << (dataset.materialize_fields().size() == dataset.fields().size()) << "\n";

  return 0;
}
```

Output

```text
materialize_fields().size() == fields().size(): true
```

## See Also

- [`fields`](fields.md) -- the `noexcept`, reference-returning
  equivalent.
- [`DataLoader`](../DataLoader.md) -- the actual consumer.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="get.html">
    <span class="label">Previous</span>
    <span class="title">get</span>
  </a>
  <a class="next-link" href="size.html">
    <span class="label">Next</span>
    <span class="title">size</span>
  </a>
</div>
<!-- apiref:nav:end -->
