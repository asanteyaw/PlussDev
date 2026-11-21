<!-- pluss-hand-authored -->

# Shelf::sort

[Back to Shelf API Reference](../../shelf_api_reference)

## Signature

```cpp
ShelfSortBuilder Shelf::sort(bool ascending = true, bool nulls_last = true) const
```

## What It Does

Starts sorting the values of the shelf. `sort` itself does not reorder anything: it binds the shelf, the direction and the
`NULL` placement, and returns a [`ShelfSortBuilder`](../../shelf_sort_accessor_api_reference) whose
[`asc`](../shelf-sort/asc.md), [`desc`](../shelf-sort/desc.md) or [`done`](../shelf-sort/done.md) produces the sorted shelf.
The builder also converts to `Shelf` on its own.

## Parameters

**ascending** (`bool`, default `true`)

The direction used by [`done`](../shelf-sort/done.md) and by the implicit conversion. `asc()` and `desc()` ignore it.

**nulls_last** (`bool`, default `true`)

Where `NULL` values go: at the end when `true`, at the start when `false`.

## Returns

`ShelfSortBuilder`
    The binder. Call [`asc`](../shelf-sort/asc.md), [`desc`](../shelf-sort/desc.md) or [`done`](../shelf-sort/done.md) on it.

## Examples

Example 1. Sort ascending, descending and with the builder's own direction.

```cpp
using namespace pluss::table;

auto values = Shelf(pluss::make_series({30.0, 10.0, 20.0}));

std::cout << values.sort().asc();
std::cout << values.sort().desc();
std::cout << values.sort(false).done();
```

### Output

```text
0  10
1  20
2  30
ptype: Series
0  30
1  20
2  10
ptype: Series
0  30
1  20
2  10
ptype: Series
```

## Notes

- Every method of the builder has its own page in the [`ShelfSortBuilder` reference](../../shelf_sort_accessor_api_reference).

## See Also

- [`TimeSeriesAccessor::sort`](../shelf-ts/sort.md): order values by a separate time axis.
