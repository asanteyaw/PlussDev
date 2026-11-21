<!-- pluss-hand-authored -->

# ShelfSortBuilder::done

[Back to ShelfSortBuilder API Reference](../../shelf_sort_accessor_api_reference)

## Signature

```cpp
Shelf ShelfSortBuilder::done() const
```

## What It Does

Finishes the sort started by [`Shelf::sort`](../shelf/sort.md) and returns the sorted shelf in the direction and `NULL` placement given to `Shelf::sort`. Use `done()` when `Shelf::sort` already carried the direction. The builder also converts to `Shelf` implicitly, which does the same.

Sorting works for numeric, text and time shelves. `NULL` values are placed last by default, or first when `Shelf::sort`
was called with `nulls_last = false`. The sort is applied to values, so the result is a new shelf and the original is
unchanged.

## Parameters

`None` (the direction and the `NULL` placement were given to `Shelf::sort`).

## Returns

`Shelf`
    A new shelf with the same length and kind, ordered as described.

## Examples

Example 1. Finish with the direction given to `sort`.

```cpp
using namespace pluss::table;

auto values = Shelf(pluss::make_series({30.0, 10.0, 20.0}));

std::cout << values.sort(false).done();
```

### Output

```text
0  30
1  20
2  10
ptype: Series
```

Example 2. Finish with the default direction (ascending).

```cpp
using namespace pluss::table;

auto values = Shelf(pluss::make_series({30.0, 10.0, 20.0}));

std::cout << values.sort().done();
```

### Output

```text
0  10
1  20
2  30
ptype: Series
```

Example 3. The builder converts to a `Shelf` on its own.

```cpp
using namespace pluss::table;

auto values = Shelf(pluss::make_series({30.0, 10.0, 20.0}));

Shelf sorted = values.sort();
std::cout << sorted;
```

### Output

```text
0  10
1  20
2  30
ptype: Series
```

## Notes

- Use `done()` when `Shelf::sort` already carried the direction. The builder also converts to `Shelf` implicitly, which does the same.
- To order values by a separate time axis, use [`TimeSeriesAccessor::sort`](../shelf-ts/sort.md).

## See Also

- [`Shelf::sort`](../shelf/sort.md): starts the builder.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="desc.html">
    <span class="label">Previous</span>
    <span class="title">desc</span>
  </a>
</div>
