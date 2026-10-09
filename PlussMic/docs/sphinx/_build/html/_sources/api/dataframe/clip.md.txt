<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6e8e7ea49d38 -->

# DataFrame::clip

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::clip(double min_value, double max_value) const -> DataFrame
```

## What It Does

Clamps every numeric feature on the frame to the closed range `[min_value, max_value]`:
a value below `min_value` becomes `min_value`, a value above `max_value` becomes
`max_value`, and everything already inside the range is left untouched. This is a
whole-frame elementwise operation — there is no per-feature bounds parameter, so `clip()`
applies one shared range across every numeric column on the frame in a single call.

The common use is capping outliers before a summary or a model input — a small number of
extreme values (a data-entry typo, a sensor spike) can otherwise dominate a `mean()` or a
`std()` far out of proportion to how much they should matter.

## Parameters

**min_value** (`double`)

Lower bound. Any numeric cell below this value is replaced with `min_value`.

**max_value** (`double`)

Upper bound. Any numeric cell above this value is replaced with `max_value`.

## Returns

`DataFrame`
    A new frame with every numeric feature clamped into `[min_value, max_value]`.
    Non-numeric features (`Note`, `Timeline`, `TimeIndex`) and null cells are left
    exactly as they were.

## Examples

Example 1. Cap an outlier-prone price column at 500 before summarizing it.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"order_id",   Slate(pluss::make_series({101, 102, 103, 104, 105, 106}))},
    {"city",       Slate(pluss::make_note({"Accra", "Accra", "Tema", "Tema", "Kumasi", "Kumasi"}))},
    {"units",      Slate(pluss::make_series({2, 5, 1, 3, 4, 2}))},
    {"unit_price", Slate(pluss::make_series({1200.0, 45.5, 1150.0, 320.0, 42.0, std::numeric_limits<double>::quiet_NaN()}))},
    {"discount",   Slate(pluss::make_series({0.1, 0.05, std::numeric_limits<double>::quiet_NaN(), 0.08, 0.05, 0.12}))},
});

std::cout << orders;
```

### Output (before)

```text
====================================================
     | order_id  city    units  unit_price  discount
----------------------------------------------------
0    | 101       Accra   2      1200        0.1     
1    | 102       Accra   5      45.5        0.05    
2    | 103       Tema    1      1150        NULL    
3    | 104       Tema    3      320         0.08    
4    | 105       Kumasi  4      42          0.05    
5    | 106       Kumasi  2      NULL        0.12    
----------------------------------------------------
[6 cs x 5 feat]
```

```cpp
auto capped = orders.clip(0.0, 500.0);
std::cout << capped;
```

### Output (after)

```text
====================================================
     | order_id  city    units  unit_price  discount
----------------------------------------------------
0    | 101       Accra   2      500         0.1     
1    | 102       Accra   5      45.5        0.05    
2    | 103       Tema    1      500         NULL    
3    | 104       Tema    3      320         0.08    
4    | 105       Kumasi  4      42          0.05    
5    | 106       Kumasi  2      NULL        0.12    
----------------------------------------------------
[6 cs x 5 feat]
```

Only `unit_price`'s two outliers (`1200` at case `0`, `1150` at case `2`) changed, both
clamped down to `500`. Every other numeric feature was already inside `[0, 500]`, so
`order_id`, `units`, and `discount` are identical to the source. The `NULL` at case `5`
stayed `NULL` — `clip()` clamps observed values, it never invents a replacement for a
missing one — and `city`, a `Note` feature, was never a candidate for clamping in the
first place.

## Notes

- `clip()` applies one shared `[min_value, max_value]` range to **every** numeric
  feature on the frame. If different features genuinely need different bounds, clip them
  one at a time via [`assign()`](assign.md) with a per-feature expression instead of a
  single `clip()` call.
- Nulls pass through unchanged — `clip()` is not a substitute for
  [`fillnull()`](fillnull.md).
- `min_value` should not exceed `max_value`; passing a backwards range collapses every
  numeric cell to a single value, which is rarely the intent.

## See Also

- [`DataFrame::round`](round.md): a different elementwise numeric cleanup, rounding
  instead of clamping.
- [`DataFrame::fillnull`](fillnull.md): handle missing values, which `clip()`
  deliberately leaves alone.
- [`DataFrame::replace`](replace.md): swap specific values rather than clamping a whole
  range.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cast.html">
    <span class="label">Previous</span>
    <span class="title">cast</span>
  </a>
  <a class="next-link" href="combine.html">
    <span class="label">Next</span>
    <span class="title">combine</span>
  </a>
</div>
