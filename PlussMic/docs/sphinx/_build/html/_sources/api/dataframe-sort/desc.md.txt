<!-- pluss-hand-authored -->

# DataFrameSortBuilder::desc

[Back to DataFrameSortBuilder API Reference](../../dataframe_sort_accessor_api_reference)

## Signature

```cpp
DataFrameSortBuilder DataFrameSortBuilder::desc() const
```

## What It Does

Marks the **active key** — the most recently added key via
[`by(...)`](by.md) or [`then(...)`](then.md) — as descending.

```{important} desc() mutates only the active key, not the whole plan
Same scoping as [`asc()`](asc.md) — see that page's own important callout.
Call `.then(...)` before `.desc()` when you want the newly-appended
tie-break key to be the one flipped, not the primary key.
```

## Parameters

`None`

## Returns

`DataFrameSortBuilder`
    A new builder with the active key's direction set to descending.

## Examples

Example 1. Sort by `"sales"` descending.

```cpp
using namespace pluss::table;

auto df = DataFrame({
    {"city",  Thread(pluss::make_note({"Tema", "Accra", "Accra", "Tema"}))},
    {"sales", Thread(pluss::make_series({120.0, 100.0, 90.0, 200.0}))},
});

auto out = df.sort().by("sales").desc().yield();
std::cout << out;
```

### Output

```text
===================
     | city   sales
-------------------
3    | Tema   200  
0    | Tema   120  
1    | Accra  100  
2    | Accra  90   
-------------------
[4 cs x 2 feat]
```

## Notes

- See the important callout above.
- Requires an active key to exist — call `.by(...)`/`.then(...)` first.
- Equivalent to supplying an `ArrangeSpec{feature, /*ascending=*/false, ...}`
  directly to `by()`/`then()`, just expressed as a follow-up mutation.

## See Also

- [`DataFrameSortBuilder::asc`](asc.md): the opposite direction.
- [`DataFrameSortBuilder::then`](then.md): append a new key before flipping
  its direction, per the important callout above.
- [`DataFrame::sort`](../dataframe/sort.md): the entry point this builder is
  reached from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="by.html">
    <span class="label">Previous</span>
    <span class="title">by</span>
  </a>
  <a class="next-link" href="head.html">
    <span class="label">Next</span>
    <span class="title">head</span>
  </a>
</div>
