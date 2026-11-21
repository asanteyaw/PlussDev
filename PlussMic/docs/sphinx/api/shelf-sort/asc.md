<!-- pluss-hand-authored -->

# ShelfSortBuilder::asc

[Back to ShelfSortBuilder API Reference](../../shelf_sort_accessor_api_reference)

## Signature

```cpp
Shelf ShelfSortBuilder::asc() const
```

## What It Does

Finishes the sort started by [`Shelf::sort`](../shelf/sort.md) and returns the sorted shelf in ascending order. `asc()` overrides the direction given to `sort`, so `values.sort(false).asc()` is ascending.

Sorting works for numeric, text and time shelves. `NULL` values are placed last by default, or first when `Shelf::sort`
was called with `nulls_last = false`. The sort is applied to values, so the result is a new shelf and the original is
unchanged.

## Parameters

`None` (the direction and the `NULL` placement were given to `Shelf::sort`).

## Returns

`Shelf`
    A new shelf with the same length and kind, ordered as described.

## Examples

Example 1. Ascending.

```cpp
using namespace pluss::table;

auto values = Shelf(pluss::make_series({30.0, 10.0, 20.0}));

std::cout << values.sort().asc();
```

### Output

```text
0  10
1  20
2  30
ptype: Series
```

Example 2. Ascending, with `NULL` values last (the default).

```cpp
using namespace pluss::table;

auto values = Shelf(pluss::make_series({30.0, 10.0, 20.0, 15.0}), pluss::make_series({true, true, false, true}));

std::cout << values.sort(true, true).asc();
```

### Output

```text
0  10
1  15
2  30
3  NULL
ptype: Series
```

Example 3. Ascending, with `NULL` values first.

```cpp
using namespace pluss::table;

auto values = Shelf(pluss::make_series({30.0, 10.0, 20.0, 15.0}), pluss::make_series({true, true, false, true}));

std::cout << values.sort(true, false).asc();
```

### Output

```text
0  NULL
1  10
2  15
3  30
ptype: Series
```

## Notes

- `asc()` overrides the direction given to `sort`, so `values.sort(false).asc()` is ascending.
- To order values by a separate time axis, use [`TimeSeriesAccessor::sort`](../shelf-ts/sort.md).

## See Also

- [`Shelf::sort`](../shelf/sort.md): starts the builder.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="desc.html">
    <span class="label">Next</span>
    <span class="title">desc</span>
  </a>
</div>
