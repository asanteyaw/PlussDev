<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 8d18c677161f -->

# DataFrame::bottom

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame DataFrame::bottom(std::string_view feature, std::int64_t n) const
```

## What It Does

Returns the `n` rows with the smallest values in `feature`, with every other feature
carried along unchanged — the ascending counterpart to [`top()`](top.md). This is
exactly `arrange({feature}, /*ascending=*/true)` followed by
[`head(n)`](head.md).

Missing values in `feature` always sort to the very end — even here, in the ascending
case — so a row with a missing `feature` value can only appear in the result if `n` is
large enough to reach all the way down past every real value.

## Parameters

**feature** (`std::string_view`)

The name of the feature to rank by, smallest first.

**n** (`std::int64_t`)

How many bottom-ranked rows to return. Values larger than `ncases()` return the whole
frame, sorted, rather than erroring.

## Returns

`DataFrame`
    A new frame with `n` rows (or fewer, if the source frame is smaller), sorted by
    `feature` in ascending order, with every feature preserved.

## Examples

Example 1. Get the three lowest-priced orders.

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

auto cheapest = orders.bottom("unit_price", 3);
std::cout << cheapest;
```

### Output

```text
================================================================
     | order_id  city    product     units  unit_price  discount
----------------------------------------------------------------
4    | 105       Kumasi  Headphones  4      42          0.05    
1    | 102       Accra   Headphones  5      45.5        0.05    
3    | 104       Tema    Monitor     3      320         0.08    
----------------------------------------------------------------
[3 cs x 6 feat]
```

The three rows with the smallest `unit_price` values (`42`, `45.5`, `320`) come back,
each with every feature intact. Case `5`, whose `unit_price` is missing, does not
appear even though ascending order would otherwise put a missing value "first" in a
naive sort — missing values are always ranked last here too, so they only surface if
`n` is large enough to reach past every real observation.

## Notes

- `bottom()` is exactly `arrange({feature}, true).head(n)`; reach for
  `arrange()` directly if you need more than one sort key.
- A missing value in `feature` sorts to the very end regardless of ascending or
  descending direction — this is the same nulls-last rule `top()` follows, not a
  nulls-first rule for the ascending case.
- For the largest values instead of the smallest, use [`top()`](top.md), which shares
  this exact signature.

## See Also

- [`DataFrame::top`](top.md): the descending counterpart, returning the largest `n`
  rows by `feature`.
- [`DataFrame::arrange`](arrange.md): the general sorting method this is built
  on, supporting multiple keys and mixed sort directions.
- [`DataFrame::head`](head.md): the plain positional prefix `bottom()` applies after
  sorting.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="bfill.html">
    <span class="label">Previous</span>
    <span class="title">bfill</span>
  </a>
  <a class="next-link" href="bucketize.html">
    <span class="label">Next</span>
    <span class="title">bucketize</span>
  </a>
</div>
