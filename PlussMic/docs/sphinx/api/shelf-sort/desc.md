<!-- pluss-hand-authored -->

# ShelfSortBuilder::desc

[Back to ShelfSortBuilder API Reference](../../shelf_sort_accessor_api_reference)

## Signature

```cpp
Shelf ShelfSortBuilder::desc() const
```

## What It Does

Finishes the sort started by [`Shelf::sort`](../shelf/sort.md) and returns the sorted shelf in descending order. `desc()` overrides the direction given to `sort`, so `values.sort(true).desc()` is descending.

Sorting works for numeric, text and time shelves. `NULL` values are placed last by default, or first when `Shelf::sort`
was called with `nulls_last = false`. The sort is applied to values, so the result is a new shelf and the original is
unchanged.

## Parameters

`None` (the direction and the `NULL` placement were given to `Shelf::sort`).

## Returns

`Shelf`
    A new shelf with the same length and kind, ordered as described.

## Examples

Example 1. Descending.

```cpp
using namespace pluss::table;

auto values = Shelf(pluss::make_series({30.0, 10.0, 20.0}));

std::cout << values.sort().desc();
```

### Output

```text
0  30
1  20
2  10
ptype: Series
```

Example 2. Descending text.

```cpp
using namespace pluss::table;

auto words = Shelf(pluss::make_note({"pear", "apple", "fig"}));

std::cout << words.sort().desc();
```

### Output

```text
0  pear
1  fig
2  apple
ptype: Note
```

Example 3. `desc()` overrides the direction given to `sort`.

```cpp
using namespace pluss::table;

auto values = Shelf(pluss::make_series({30.0, 10.0, 20.0}));

std::cout << values.sort(true).desc();
```

### Output

```text
0  30
1  20
2  10
ptype: Series
```

## Notes

- `desc()` overrides the direction given to `sort`, so `values.sort(true).desc()` is descending.
- To order values by a separate time axis, use [`TimeSeriesAccessor::sort`](../shelf-ts/sort.md).

## See Also

- [`Shelf::sort`](../shelf/sort.md): starts the builder.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="asc.html">
    <span class="label">Previous</span>
    <span class="title">asc</span>
  </a>
  <a class="next-link" href="done.html">
    <span class="label">Next</span>
    <span class="title">done</span>
  </a>
</div>
