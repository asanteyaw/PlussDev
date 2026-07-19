<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 51b5f33183c3 -->

# DataFrame::take

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::take(pluss::RangeView<std::int64_t> indices) const -> DataFrame
```

## What It Does

Returns a new frame built from exactly the case positions listed in `indices`, in
exactly the order given. Unlike [`slice()`](slice.md), `indices` doesn't need to be
contiguous or ascending — you can reorder rows, reverse a frame, or repeat a row
multiple times by listing its position more than once. This is the general-purpose
row-selection primitive several other methods ([`slice()`](slice.md),
[`head()`](head.md), [`tail()`](tail.md), [`sample()`](sample.md),
[`shuffle()`](shuffle.md)) are built on top of internally.

Every requested position is resolved with Python-style negative-index support (`-1`
means the last case), but — unlike `slice()`'s clamping behavior — an out-of-range
position (after negative normalization) is a validation error here, not silently
clamped. Case labels on the result follow the same positions and order as `indices`:
row `5` keeps its original label `5` even if it now appears first in the result.

## Parameters

**indices** (`pluss::RangeView<std::int64_t>`)

The case positions to select, in the exact order the result should have. Negative
values are interpreted relative to the end of the frame. The same position may appear
more than once — the corresponding row is duplicated once per occurrence.

## Returns

`DataFrame`
    A new frame with one row per entry in `indices`, in that exact order, holding
    every feature of the source frame and the original case label of each selected
    position.

## Examples

Example 1. Reorder and duplicate rows in a single call.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"order_id",   Thread(pluss::make_series({101, 102, 103, 104, 105, 106}))},
    {"city",       Thread(pluss::make_note({"Accra", "Accra", "Tema", "Tema", "Kumasi", "Kumasi"}))},
    {"product",    Thread(pluss::make_note({"Laptop", "Headphones", "Laptop", "Monitor", "Headphones", "Monitor"}))},
    {"units",      Thread(pluss::make_series({2, 5, 1, 3, 4, 2}))},
    {"unit_price", Thread(pluss::make_series({1200.0, 45.5, 1150.0, 320.0, 42.0, std::numeric_limits<double>::quiet_NaN()}))},
    {"discount",   Thread(pluss::make_series({0.1, 0.05, std::numeric_limits<double>::quiet_NaN(), 0.08, 0.05, 0.12}))},
});

auto picked = orders.take({5, 0, 0, 2});
std::cout << picked;
```

### Output

```text
=============================================================
     | order_id  city    product  units  unit_price  discount
-------------------------------------------------------------
5    | 106       Kumasi  Monitor  2      NULL        0.12    
0    | 101       Accra   Laptop   2      1200        0.1     
0    | 101       Accra   Laptop   2      1200        0.1     
2    | 103       Tema    Laptop   1      1150        NULL    
-------------------------------------------------------------
[4 cs x 6 feat]
```

The result has four rows even though only three distinct positions were requested,
because case `0` was listed twice — it appears twice in the output, each copy keeping
the label `0`. The row order follows `indices` exactly, not the source frame's order.

Example 2. Reverse the entire frame.

```cpp
std::vector<std::int64_t> reversed_order{5, 4, 3, 2, 1, 0};
auto reversed = orders.take(reversed_order);
std::cout << reversed;
```

### Output

```text
================================================================
     | order_id  city    product     units  unit_price  discount
----------------------------------------------------------------
5    | 106       Kumasi  Monitor     2      NULL        0.12    
4    | 105       Kumasi  Headphones  4      42          0.05    
3    | 104       Tema    Monitor     3      320         0.08    
2    | 103       Tema    Laptop      1      1150        NULL    
1    | 102       Accra   Headphones  5      45.5        0.05    
0    | 101       Accra   Laptop      2      1200        0.1     
----------------------------------------------------------------
[6 cs x 6 feat]
```

Every row is present exactly once, but the case order — and the case labels — run
from `5` down to `0`.

## Notes

- Unlike `slice()`, an out-of-range position in `indices` (after negative-index
  normalization) is a validation error, not silently clamped.
- `take()` is the primitive `slice()`, `head()`, `tail()`, `sample()`, and
  `shuffle()`/`reshuffle()` all delegate to internally, so its result-shape guarantees
  (feature order, dtypes, original case labels) apply to all of them.
- Repeating a position produces a row-level duplicate in the result, including a
  duplicated case label — useful for bootstrapped resampling or building a synthetic
  frame from an existing one's rows.

## See Also

- [`DataFrame::slice`](slice.md): the contiguous-range special case of this method.
- [`DataFrame::sample`](sample.md): a random subset of positions, built on `take()`.
- [`DataFrame::case_study`](case_study.md): a richer, selector-based way to pick rows
  (and optionally features) by position, label, or boolean mask.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tail.html">
    <span class="label">Previous</span>
    <span class="title">tail</span>
  </a>
  <a class="next-link" href="tally.html">
    <span class="label">Next</span>
    <span class="title">tally</span>
  </a>
</div>
