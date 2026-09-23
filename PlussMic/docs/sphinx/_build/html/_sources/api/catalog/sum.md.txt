<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 72f9ad4b5627 -->

# Catalog::sum

[Back to Catalog API Reference](../../catalog_api_reference)

## Signatures

```cpp
Catalog::sum() const -> Catalog
Catalog::sum(pluss::kindle::ReduceAxis axis) const -> Catalog
Catalog::sum(const pluss::kindle::CatalogReduceOptions& options) const -> Catalog
```

## What It Does

Reduces numeric streams in one of two ways:

- `ReduceAxis::Record` (the default) reduces each stream independently and returns one
  scalar stream per key.
- `ReduceAxis::Key` reduces across the selected keys row-by-row and returns a one-key
  catalog named `sum`.

For cross-key reduction, `Catalog` aligns the selected streams with its table bridge and
then delegates the numeric reduction to the table stack.

## Options

`CatalogReduceOptions` supports:

- `axis`: `ReduceAxis::Record` or `ReduceAxis::Key`
- `align`: how unequal stream lengths are handled for cross-key reduction
- `align_by`: positional vs label alignment for cross-key reduction
- `skipnull`: whether `NULL` values are ignored
- `fill_scalar`: optional scalar fill applied before cross-key reduction

## Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "a"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "b"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
});

auto by_stream = ct.sum();
auto by_key = ct.sum(pluss::kindle::ReduceAxis::Key);
```

## Notes

- Cross-key reduction currently requires `Series` streams.
- With `ReduceAxis::Key`, ragged stream lengths are governed by `align`.

## See Also

- [`Catalog::mean`](mean.md)
- [`Catalog::min`](min.md)
- [`Catalog::max`](max.md)
- [`Catalog::var`](var.md)
- [`Catalog::std`](std.md)

