<!-- pluss-hand-authored -->

# DataFrameSortBuilder::head

[Back to DataFrameSortBuilder API Reference](../../dataframe_sort_accessor_api_reference)

## Signature

```cpp
DataFrame DataFrameSortBuilder::head(std::int64_t n) const
```

## What It Does

Executes the plan and returns only the top-ranked `n` rows — a "top-N"
shorthand for [`yield().slice(0, n)`](../dataframe/slice.md) that (per the
underlying implementation) can avoid materializing the full sorted frame
just to discard most of it.

```{important} A terminal method — unknown-feature validation fires here
Same as [`yield()`](yield.md)/[`order()`](order.md)/[`tail()`](tail.md) —
calling `head()` is what actually executes the plan. See [`by()`](by.md)'s
own edge-case example.
```

## Parameters

**n** (`std::int64_t`)
    How many top-ranked rows to return. Must be non-negative.

## Returns

`DataFrame`
    The first `n` rows under the planned ordering.

## Examples

Example 1. The two highest-`sales` rows.

```cpp
using namespace pluss::table;

auto df = DataFrame({
    {"city",  Thread(pluss::make_note({"Tema", "Accra", "Accra", "Tema"}))},
    {"sales", Thread(pluss::make_series({120.0, 100.0, 90.0, 200.0}))},
});

auto top2 = df.sort().by("sales").desc().head(2);
std::cout << top2;
```

### Output

```text
==================
     | city  sales
------------------
3    | Tema  200  
0    | Tema  120  
------------------
[2 cs x 2 feat]
```

## Notes

- See the important callout above.
- `n` must be non-negative.
- Uses the same planned ordering `yield()` would produce — the top rows
  under that plan, not simply the first `n` rows of the unsorted frame.

## See Also

- [`DataFrameSortBuilder::tail`](tail.md): the bottom-ranked counterpart.
- [`DataFrameSortBuilder::yield`](yield.md): the full sorted frame, with no
  row limit.
- [`DataFrame::sort`](../dataframe/sort.md): the entry point this builder is
  reached from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="desc.html">
    <span class="label">Previous</span>
    <span class="title">desc</span>
  </a>
  <a class="next-link" href="nulls_first.html">
    <span class="label">Next</span>
    <span class="title">nulls_first</span>
  </a>
</div>
