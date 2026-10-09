<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: be88ea043bdd -->

# DataFrame::top

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::top(std::string_view feature, std::int64_t n) const -> DataFrame
```

## What It Does

Returns the `n` rows with the largest values in `feature`, with every other feature
carried along unchanged — the whole row moves together, not just the sorted column.
This is exactly `arrange({feature}, /*ascending=*/false)` followed by
[`head(n)`](head.md); it exists as a single call for the extremely common
"give me the top N by X" query.

Missing values in `feature` always sort to the very end, regardless of the descending
direction — a row with a missing `feature` value can only appear in the result if `n`
is large enough to reach that far down the ranking.

## Parameters

**feature** (`std::string_view`)

The name of the feature to rank by, largest first.

**n** (`std::int64_t`)

How many top-ranked rows to return. Values larger than `ncases()` return the whole
frame, sorted, rather than erroring.

## Returns

`DataFrame`
    A new frame with `n` rows (or fewer, if the source frame is smaller), sorted by
    `feature` in descending order, with every feature preserved.

## Examples

Example 1. Get the three highest-priced orders.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"order_id",   Slate(pluss::make_series({101, 102, 103, 104, 105, 106}))},
    {"city",       Slate(pluss::make_note({"Accra", "Accra", "Tema", "Tema", "Kumasi", "Kumasi"}))},
    {"product",    Slate(pluss::make_note({"Laptop", "Headphones", "Laptop", "Monitor", "Headphones", "Monitor"}))},
    {"units",      Slate(pluss::make_series({2, 5, 1, 3, 4, 2}))},
    {"unit_price", Slate(pluss::make_series({1200.0, 45.5, 1150.0, 320.0, 42.0, std::numeric_limits<double>::quiet_NaN()}))},
    {"discount",   Slate(pluss::make_series({0.1, 0.05, std::numeric_limits<double>::quiet_NaN(), 0.08, 0.05, 0.12}))},
});

auto priciest = orders.top("unit_price", 3);
std::cout << priciest;
```

### Output

```text
============================================================
     | order_id  city   product  units  unit_price  discount
------------------------------------------------------------
0    | 101       Accra  Laptop   2      1200        0.1     
2    | 103       Tema   Laptop   1      1150        NULL    
3    | 104       Tema   Monitor  3      320         0.08    
------------------------------------------------------------
[3 cs x 6 feat]
```

The three rows with the largest `unit_price` values (`1200`, `1150`, `320`) come back
with every feature intact — `discount`'s own missing value at case `2` rides along
unaffected, since the ranking only cares about `unit_price`. Case `5`, whose
`unit_price` is missing, does not appear: missing values are always ranked last, no
matter how few rows are requested.

## Notes

- `top()` is exactly `arrange({feature}, false).head(n)`; reach for `arrange()`
  directly if you need more than one sort key or an ascending mix.
- A missing value in `feature` sorts to the very end regardless of direction — it can
  only appear in a `top()` result if `n` is large enough to include it.
- For the smallest values instead of the largest, use [`bottom()`](bottom.md), which
  shares this exact signature.

## See Also

- [`DataFrame::bottom`](bottom.md): the ascending counterpart, returning the smallest
  `n` rows by `feature`.
- [`DataFrame::arrange`](arrange.md): the general sorting method this is built
  on, supporting multiple keys and mixed sort directions.
- [`DataFrame::head`](head.md): the plain positional prefix `top()` applies after
  sorting.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_string.html">
    <span class="label">Previous</span>
    <span class="title">to_string</span>
  </a>
  <a class="next-link" href="transform.html">
    <span class="label">Next</span>
    <span class="title">transform</span>
  </a>
</div>
