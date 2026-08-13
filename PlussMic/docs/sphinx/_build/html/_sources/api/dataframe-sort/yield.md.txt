<!-- pluss-hand-authored -->

# DataFrameSortBuilder::yield

[Back to DataFrameSortBuilder API Reference](../../dataframe_sort_accessor_api_reference)

## Signature

```cpp
DataFrame DataFrameSortBuilder::yield() const
```

## What It Does

Executes the sort plan and returns the fully sorted `DataFrame` — the
terminal that corresponds most directly to
[`DataFrame::arrange(...)`](../dataframe/arrange.md)'s direct execute-now
form, reached instead through the reusable builder.

```{important} A terminal method — unknown-feature validation fires here
Same as [`order()`](order.md)/[`head()`](head.md)/[`tail()`](tail.md) —
calling `yield()` is what actually executes the plan and validates every
feature name supplied to `.by(...)`/`.then(...)`. See
[`by()`](by.md)'s own edge-case example.
```

## Parameters

`None`

## Returns

`DataFrame`
    A new frame with rows reordered by the planned key(s).

## Examples

Example 1. Build a multi-key plan, then materialize it with `yield()`.

```cpp
using namespace pluss::table;

auto df = DataFrame({
    {"city",  Thread(pluss::make_note({"Tema", "Accra", "Accra", "Tema"}))},
    {"sales", Thread(pluss::make_series({120.0, 100.0, 90.0, 200.0}))},
});

auto out = df.sort().by("city").then("sales").desc().yield();
std::cout << out;
```

### Output

```text
===================
     | city   sales
-------------------
1    | Accra  100  
2    | Accra  90   
3    | Tema   200  
0    | Tema   120  
-------------------
[4 cs x 2 feat]
```

## Notes

- See the important callout above.
- Equivalent in result to calling
  [`order()`](order.md) followed by [`take()`](../dataframe/take.md)
  yourself — `yield()` is the direct one-call path when you don't need the
  raw permutation for anything else.

## See Also

- [`DataFrameSortBuilder::order`](order.md): materializes the permutation
  instead of applying it directly.
- [`DataFrameSortBuilder::head`](head.md) / [`tail()`](tail.md): execute the
  plan and keep only the top/bottom `n` rows.
- [`DataFrame::arrange`](../dataframe/arrange.md): the direct execute-now
  sorter this method corresponds to.
- [`DataFrame::sort`](../dataframe/sort.md): the entry point this builder is
  reached from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="within.html">
    <span class="label">Previous</span>
    <span class="title">within</span>
  </a>
</div>
