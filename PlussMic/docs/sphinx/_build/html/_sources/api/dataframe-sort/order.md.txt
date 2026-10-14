<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 04a50b2b6a19 -->

# DataFrameSortBuilder::order

[Back to DataFrameSortBuilder API Reference](../../dataframe_sort_accessor_api_reference)

## Signature

```cpp
Slate DataFrameSortBuilder::order() const
```

## What It Does

Executes the plan and materializes it as a `Slate` of `Int64` **row
positions** — the reusable permutation artifact, rather than the sorted
frame itself. Feeding this into [`DataFrame::take`](../dataframe/take.md)
(on the same frame, or any other frame/slate that shares its row
alignment) reproduces the sorted order.

```{important} A terminal method — this is where unknown-feature validation actually fires
Like [`yield()`](yield.md)/[`head()`](head.md)/[`tail()`](tail.md), calling
`order()` is what actually executes the plan — an unknown feature name
supplied to an earlier `.by(...)`/`.then(...)` call only throws here. See
[`by()`](by.md)'s own edge-case example.
```

## Parameters

`None`

## Returns

`Slate`
    An `Int64` `Series`-kind slate of row positions, in sorted order.

## Examples

Example 1. Materialize the permutation, then apply it via
[`take()`](../dataframe/take.md) — useful when several aligned objects need
the exact same ordering.

```cpp
using namespace pluss::table;

auto df = DataFrame({
    {"city",  Slate(pluss::make_note({"Tema", "Accra", "Accra", "Tema"}))},
    {"sales", Slate(pluss::make_series({120.0, 100.0, 90.0, 200.0}))},
});

auto ord = df.sort().by("sales").desc().order();
std::cout << ord;

auto taken = df.take(ord);
std::cout << "take(order()):\n" << taken;
```

### Output

```text
0  3
1  0
2  1
3  2

ptype: Series
take(order()):
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

`df.take(ord)` reproduces exactly what `df.sort().by("sales").desc().yield()`
would have returned directly — `order()` is useful specifically when the
same permutation needs to be applied to more than one object.

## Notes

- See the important callout above.
- The result's own case labels are the source positions in sorted order —
  the same convention `take()` itself follows.

## See Also

- [`DataFrameSortBuilder::yield`](yield.md): executes and returns the
  sorted frame directly, when you don't need the raw permutation.
- [`DataFrame::take`](../dataframe/take.md): the row-selection primitive
  this permutation feeds into.
- [`DataFrame::sort`](../dataframe/sort.md): the entry point this builder is
  reached from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nulls_last.html">
    <span class="label">Previous</span>
    <span class="title">nulls_last</span>
  </a>
  <a class="next-link" href="tail.html">
    <span class="label">Next</span>
    <span class="title">tail</span>
  </a>
</div>
