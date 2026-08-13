<!-- pluss-hand-authored -->

# DataFrameSortBuilder::asc

[Back to DataFrameSortBuilder API Reference](../../dataframe_sort_accessor_api_reference)

## Signature

```cpp
DataFrameSortBuilder DataFrameSortBuilder::asc() const
```

## What It Does

Marks the **active key** — the most recently added key via
[`by(...)`](by.md) or [`then(...)`](then.md) — as ascending.

```{important} asc() mutates only the active key, not the whole plan
Calling `.asc()` after `.by("a").then("b")` changes only `"b"`'s direction;
`"a"` keeps whatever direction it already had. Call `.asc()`/`.desc()`
immediately after the key you mean to change, before adding the next one.
```

## Parameters

`None`

## Returns

`DataFrameSortBuilder`
    A new builder with the active key's direction set to ascending.

## Examples

Example 1. Set the primary key explicitly to ascending (the default
direction already, shown here for symmetry with [`desc()`](desc.md)).

```cpp
using namespace pluss::table;

auto df = DataFrame({
    {"city",  Thread(pluss::make_note({"Tema", "Accra", "Accra", "Tema"}))},
    {"sales", Thread(pluss::make_series({120.0, 100.0, 90.0, 200.0}))},
});

auto out = df.sort().by("sales").asc().yield();
std::cout << out;
```

### Output

```text
===================
     | city   sales
-------------------
2    | Accra  90   
1    | Accra  100  
0    | Tema   120  
3    | Tema   200  
-------------------
[4 cs x 2 feat]
```

## Notes

- See the important callout above.
- Requires an active key to exist — call `.by(...)`/`.then(...)` first.
- `sort()`'s own `ascending` parameter already defaults every key to
  ascending; `asc()` is mainly useful to reverse a prior `.desc()` on the
  active key, or when `sort(false)` set a descending default that this
  specific key should override back to ascending.

## See Also

- [`DataFrameSortBuilder::desc`](desc.md): the opposite direction.
- [`DataFrameSortBuilder::nulls_first`](nulls_first.md) /
  [`nulls_last()`](nulls_last.md): the companion null-placement mutators.
- [`DataFrame::sort`](../dataframe/sort.md): the entry point this builder is
  reached from, whose `ascending` parameter sets every key's default.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="by.html">
    <span class="label">Next</span>
    <span class="title">by</span>
  </a>
</div>
