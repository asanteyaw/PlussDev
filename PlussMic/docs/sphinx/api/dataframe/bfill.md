<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6bd3cd38e266 -->

# DataFrame::bfill

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame DataFrame::bfill(std::int64_t limit = -1) const

DataFrame DataFrame::bfill(const FillOptions& options) const
```

## What It Does

Backward-fills missing values: each null cell is replaced by the next *observed* value
below it in the same feature, propagating upward. `bfill()` works on every `Slate`
kind PlussMic supports — `Series`, `Note`, `Timeline`, and `TimeIndex` — not just
numeric features, since propagating an existing observation doesn't require numeric
semantics the way interpolation does.

A trailing run of nulls with no later observed value in the same feature is left
`NULL` — there is nothing below it to borrow from.

## Parameters

**limit** (`std::int64_t`, default `-1`)

Maximum number of consecutive nulls to fill per run, counted starting from the null
closest to the next observed value and moving away from it.

- `-1` (the default): no limit, fill every null that has a later observed value.
- A non-negative value: fill at most that many nulls per run; any remaining nulls
  further from the anchor stay `NULL`.

**options** (`FillOptions`)

Structured form for targeting a feature subset:

- `features`: the feature names to fill. Empty (the default) targets every feature on
  the frame.
- `limit`: same meaning as the simple overload's `limit`.

## Returns

`DataFrame`
    A new frame, same shape as the source, with nulls backward-filled in the targeted
    features. Untargeted features are unchanged.

## Examples

Example 1. Backward-fill a `Note` feature with no run-length limit.

```cpp
using namespace pluss::table;

auto city_valid = pluss::make_series({true, false, false, true, false, true}, pluss::ScalarType::Bool);
auto orders = DataFrame({
    {"city", Slate(pluss::make_note({"Accra", "", "", "Tema", "", "Kumasi"}), city_valid)},
});

std::cout << orders;
```

### Output (before)

```text
=============
     | city  
-------------
0    | Accra 
1    | NULL  
2    | NULL  
3    | Tema  
4    | NULL  
5    | Kumasi
-------------
[6 cs x 1 feat]
```

```cpp
auto filled = orders.bfill();
std::cout << filled;
```

### Output (after)

```text
=============
     | city  
-------------
0    | Accra 
1    | Tema  
2    | Tema  
3    | Tema  
4    | Kumasi
5    | Kumasi
-------------
[6 cs x 1 feat]
```

Cases `1` and `2` both borrowed `"Tema"` (the next observed value below them), and case
`4` borrowed `"Kumasi"`. Case `0` was never null, so it is untouched.

Example 2. Cap the run length with `limit = 1`.

```cpp
auto partially_filled = orders.bfill(1);
std::cout << partially_filled;
```

### Output

```text
=============
     | city  
-------------
0    | Accra 
1    | NULL  
2    | Tema  
3    | Tema  
4    | Kumasi
5    | Kumasi
-------------
[6 cs x 1 feat]
```

With `limit = 1`, only one null per run gets filled — the one closest to the observed
value it borrows from. Case `2` (adjacent to `"Tema"`) is filled, but case `1` (one step
further from the anchor) is not, because the run already used up its one allowed fill.
Case `4`'s run is only one null long, so it fills either way.

## Notes

- `bfill()` propagates upward feature by feature; values never cross feature
  boundaries.
- Unlike [`interp()`](interp.md), `bfill()` is not restricted to numeric `Series`
  features — it works on `Note`, `Timeline`, and `TimeIndex` columns too.
- A trailing run of nulls with no later observed value in that feature stays `NULL`,
  no matter the `limit`.
- For the opposite direction (propagating the last observed value forward), see
  [`ffill()`](ffill.md). For numeric linear interpolation between two observed
  neighbors, see [`interp()`](interp.md).

## See Also

- [`DataFrame::ffill`](ffill.md): propagate the previous observed value forward
  instead of the next one backward.
- [`DataFrame::interp`](interp.md): fill numeric gaps by interpolating between
  neighbors instead of copying one of them.
- [`DataFrame::fillnull`](fillnull.md): write an explicit replacement value instead of
  borrowing a neighboring observation.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="astype.html">
    <span class="label">Previous</span>
    <span class="title">astype</span>
  </a>
  <a class="next-link" href="bottom.html">
    <span class="label">Next</span>
    <span class="title">bottom</span>
  </a>
</div>
