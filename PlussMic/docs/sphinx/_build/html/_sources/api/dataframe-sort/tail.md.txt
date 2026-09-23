<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ce526e846533 -->

# DataFrameSortBuilder::tail

[Back to DataFrameSortBuilder API Reference](../../dataframe_sort_accessor_api_reference)

## Signature

```cpp
DataFrame DataFrameSortBuilder::tail(std::int64_t n) const
```

## What It Does

Executes the plan and returns only the bottom-ranked `n` rows — the mirror
image of [`head(n)`](head.md).

```{important} A terminal method — unknown-feature validation fires here
Same as [`yield()`](yield.md)/[`order()`](order.md)/[`head()`](head.md) —
calling `tail()` is what actually executes the plan. See [`by()`](by.md)'s
own edge-case example.
```

## Parameters

**n** (`std::int64_t`)
    How many bottom-ranked rows to return. Must be non-negative.

## Returns

`DataFrame`
    The last `n` rows under the planned ordering.

## Examples

Example 1. The two lowest-ranked rows under a `"sales"`-descending plan —
i.e. the two smallest `sales` values.

```cpp
using namespace pluss::table;

auto df = DataFrame({
    {"city",  Thread(pluss::make_note({"Tema", "Accra", "Accra", "Tema"}))},
    {"sales", Thread(pluss::make_series({120.0, 100.0, 90.0, 200.0}))},
});

auto bot2 = df.sort().by("sales").desc().tail(2);
std::cout << bot2;
```

### Output

```text
===================
     | city   sales
-------------------
1    | Accra  100  
2    | Accra  90   
-------------------
[2 cs x 2 feat]
```

`tail(2)` here uses the *descending* plan, so it returns the last two rows
under that ordering — the two smallest `sales` values, not the two largest.

## Notes

- See the important callout above.
- `n` must be non-negative.
- `tail(n)` reflects the planned ordering; it is not equivalent to the
  unsorted frame's own trailing rows.

## See Also

- [`DataFrameSortBuilder::head`](head.md): the top-ranked counterpart.
- [`DataFrameSortBuilder::yield`](yield.md): the full sorted frame, with no
  row limit.
- [`DataFrame::sort`](../dataframe/sort.md): the entry point this builder is
  reached from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="order.html">
    <span class="label">Previous</span>
    <span class="title">order</span>
  </a>
  <a class="next-link" href="then.html">
    <span class="label">Next</span>
    <span class="title">then</span>
  </a>
</div>
