<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e63617ce8e28 -->

# DataFrame::case_study

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::case_study(const Case& rows) const -> DataFrame
DataFrame::case_study(const Case& rows, const Feat& features) const -> DataFrame
DataFrame::case_study(const CasePick& pick) const -> DataFrame
```

## What It Does

The general-purpose row-and-feature selection method: pick rows by position, by case
label, or by boolean mask, optionally combined with a feature projection, all in one
call that always returns a `DataFrame` — even when the selection resolves to a single
row, a single column, or a single cell. `Case` and `Feat` are small selector structs
built via their own static factory methods (`Case::at(...)`, `Feat::name(...)`, and so
on, documented below) rather than by setting their fields directly — the fields are an
implementation detail of how the selector is dispatched internally.

The third overload, taking a `CasePick`, is for a different situation: a frame that
already has one or more **named** case-label levels, set up via
[`label().set(...)`](label.md). It looks up rows by matching those named levels'
values rather than by position or by the plain default case label.

`case_study()` is the selection surface to learn well because it is the point where
PlussMic's row semantics and feature semantics meet. Many narrower methods are really
special cases of it:

- [`slice()`](slice.md): positional rows only
- [`take()`](take.md): positional rows in an explicit list
- [`filter()`](filter.md): boolean-mask rows only
- [`features()`](features.md): feature projection only
- [`zoom()`](zoom.md): rectangular positional row+feature block

If you can read a `Case` selector and a `Feat` selector fluently, you can usually
predict exactly what `case_study()` will do.

## How To Think About It

There are three independent ideas on this surface:

1. **How rows are selected**

- by position: `Case::at`, `Case::slice`, `Case::take`
- by the current case label payload: `Case::label`, `Case::labels`, `Case::span`
- by a boolean mask: `Case::where`
- by named case-label levels: `CasePick`

2. **How features are selected**

- all features: omit the second argument or pass `Feat::all()`
- by one feature: `Feat::name`, `Feat::at`
- by many features: `Feat::names`, `Feat::take`
- by a contiguous positional/name window: `Feat::slice`, `Feat::span`

3. **What shape comes back**

- always a `DataFrame`
- row order follows the row selector
- feature order follows the feature selector
- duplicate labels match all rows carrying that label
- case labels are preserved on surviving rows unless you are using `CasePick` with
  `drop = true`

That final point matters. `case_study()` does not collapse to a scalar or a `Thread`
just because you selected one row or one feature. It stays on the frame surface so you
can keep chaining frame operations naturally.

## Parameters

**rows** (`DataFrame::Case`) — built with one of:

- `Case::at(row)`: one row by position. Negative positions count from the end
  (`Case::at(-1)` is the last row).
- `Case::slice(start, stop)`: a half-open positional slice `[start, stop)`, the same
  convention [`slice()`](slice.md) uses.
- `Case::take({r1, r2, ...})`: an explicit, arbitrarily ordered (and possibly
  repeating) list of positions — the same generality [`take()`](take.md) offers.
- `Case::label(value)`: every row whose current case label equals `value`. If the same
  label appears on more than one row, **all** of them are returned, not just the
  first.
- `Case::labels({v1, v2, ...})`: several labels, in the order you list them — the
  output order follows your list, not the frame's original row order.
- `Case::span(start, stop)`: an **inclusive** case-label window, from the first row
  matching `start` through the last row matching `stop`. This is a positional
  distinction worth noting: it is inclusive on both ends, unlike `Case::slice(start,
  stop)`, which is exclusive on `stop`.
- `Case::where(bool_thread)`: every row where a boolean `Thread`/`Series` (one entry
  per case) is `true` — the same selection [`filter()`](filter.md) performs.

**features** (`DataFrame::Feat`, optional) — built with one of:

- `Feat::all()`: every feature, in source order (the default when this parameter is
  omitted entirely).
- `Feat::name(value)` / `Feat::at(position)`: one feature, by name or by position.
- `Feat::names({n1, n2, ...})` / `Feat::take({p1, p2, ...})`: several features, by
  name or by position, in the order you list them.
- `Feat::slice(start, stop)`: a half-open positional feature slice.
- `Feat::span(start, stop)`: an **inclusive** feature-name window, from `start`'s
  position through `stop`'s position (in the frame's own feature order, not
  alphabetical) — inclusive on both ends, mirroring `Case::span()`'s convention.

**pick** (`CasePick`) — a struct with:

- `label` / `value`: match rows where the named case-label level `label` equals
  `value`.
- `names` / `values`: the same idea across several named levels at once — every row
  must match all of the given level/value pairs.
- `key`: a full positional key, one value per level in the frame's own case-level
  order, when you'd rather not name each level explicitly.
- `drop` (`bool`, default `false`): when `true`, removes the matched level(s) from the
  result's case-label structure after selection, instead of keeping them (now
  redundant, since they're constant across the matched rows) on every result row.

Exactly one of `label`/`value`, `names`/`values`, or `key` should be set per call.

## Returns

`DataFrame`
    A new frame containing the matched rows (and, with the two-argument overload,
    only the selected features), in the order the selector produced them.

## When To Use It

Use `case_study()` when the row selection and the feature selection belong together as
one operation.

Typical cases:

- inspect a handful of suspicious rows together with only the columns that matter
- carve out a labeled diagnostic subframe after a join, fill, or aggregation
- look rows up by business keys promoted into the case axis via `label().set(...)`
- keep the same result as a `DataFrame` even when the selection is very small

Use a narrower method when only one axis matters:

- row-only positional work: [`slice()`](slice.md), [`take()`](take.md)
- row-only boolean work: [`filter()`](filter.md)
- feature-only work: [`features()`](features.md)
- rectangular positional block work: [`zoom()`](zoom.md)

## Examples

Example 1. Select a single row by position, including counting from the end.

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

std::cout << orders.case_study(DataFrame::Case::at(2));
std::cout << orders.case_study(DataFrame::Case::at(-1));
```

### Output

```text
===========================================================
     | order_id  city  product  units  unit_price  discount
-----------------------------------------------------------
2    | 103       Tema  Laptop   1      1150        NULL    
-----------------------------------------------------------
[1 cs x 6 feat]
=============================================================
     | order_id  city    product  units  unit_price  discount
-------------------------------------------------------------
5    | 106       Kumasi  Monitor  2      NULL        0.12    
-------------------------------------------------------------
[1 cs x 6 feat]
```

`Case::at(-1)` reaches the last row (case `5`) without needing to know `ncases()`
first.

Example 2. Select a positional slice, then an explicit reordered list combined with a
feature projection.

```cpp
using Case = DataFrame::Case;
using Feat = DataFrame::Feat;

auto middle = orders.case_study(Case::slice(1, 4));
std::cout << middle;

auto picked = orders.case_study(Case::take({0, 2, 4}), Feat::names({"city", "product"}));
std::cout << picked;
```

### Output

```text
===============================================================
     | order_id  city   product     units  unit_price  discount
---------------------------------------------------------------
1    | 102       Accra  Headphones  5      45.5        0.05    
2    | 103       Tema   Laptop      1      1150        NULL    
3    | 104       Tema   Monitor     3      320         0.08    
---------------------------------------------------------------
[3 cs x 6 feat]
=========================
     | city    product   
-------------------------
0    | Accra   Laptop    
2    | Tema    Laptop    
4    | Kumasi  Headphones
-------------------------
[3 cs x 2 feat]
```

`picked` keeps only `city` and `product`, and its rows follow `Case::take({0, 2,
4})`'s exact order — which happens to already be ascending here.

Example 3. Select by case label — including a duplicate label matching more than one
row, and a custom output order.

```cpp
auto sales = DataFrame({
    {"city",   Thread(pluss::make_note({"Accra", "Tema", "Kumasi", "Tamale"}))},
    {"region", Thread(pluss::make_note({"south", "south", "middle", "north"}))},
    {"units",  Thread(pluss::make_series({10, 7, 12, 5}))},
});
auto labeled = sales.with_cases(Thread(pluss::make_note({"A100", "B200", "B200", "D400"})));

auto b200_rows = labeled.case_study(Case::label("B200"));
std::cout << b200_rows;

auto reordered = labeled.case_study(Case::labels({"D400", "A100"}));
std::cout << reordered;
```

### Output

```text
============================
     | city    region  units
----------------------------
B200 | Tema    south   7    
B200 | Kumasi  middle  12   
----------------------------
[2 cs x 3 feat]
============================
     | city    region  units
----------------------------
D400 | Tamale  north   5    
A100 | Accra   south   10   
----------------------------
[2 cs x 3 feat]
```

`Case::label("B200")` matches **both** rows carrying that label — labels are not
required to be unique, and duplicates are never silently collapsed.
`Case::labels({"D400", "A100"})` returns exactly those two rows in the order
requested, not in the frame's original top-to-bottom order.

Example 4. Select an inclusive case-label range, combined with an inclusive
feature-name range.

```cpp
auto window = labeled.case_study(Case::span("B200", "D400"), Feat::span("city", "region"));
std::cout << window;
```

### Output

```text
=====================
     | city    region
---------------------
B200 | Tema    south 
B200 | Kumasi  middle
D400 | Tamale  north 
---------------------
[3 cs x 2 feat]
```

The row range runs from the **first** row matching `"B200"` through the **last** row
matching `"D400"`, inclusive — three rows, not two, since `"B200"` itself matches
twice. The feature range keeps `city` through `region` (positions `0` and `1` in the
frame's own feature order), excluding `units`.

Example 5. Select with a boolean mask, exactly like `filter()`, and project features
in the same call.

```cpp
auto expensive = orders.case_study(
    Case::where(orders.features("unit_price") > 100.0),
    Feat::names({"city", "product", "unit_price"}));
std::cout << expensive;
```

### Output

```text
=================================
     | city   product  unit_price
---------------------------------
0    | Accra  Laptop   1200      
2    | Tema   Laptop   1150      
3    | Tema   Monitor  320       
---------------------------------
[3 cs x 3 feat]
```

Example 6. Use `CasePick` to look up rows by a named case-label level, after
`label().set(...)` has promoted a feature into the case axis.

```cpp
auto regional = DataFrame({
    {"region", Thread(pluss::make_note({"West", "West", "East", "East"}))},
    {"sales",  Thread(pluss::make_series({10.0, 20.0, 30.0, 40.0}))},
    {"cost",   Thread(pluss::make_series({4.0, 8.0, 12.0, 16.0}))},
}).label().set("region", true);

auto west = regional.case_study(DataFrame::CasePick{.label = "region", .value = "West"});
std::cout << west;

auto east = regional.case_study(DataFrame::CasePick{.key = {"East"}});
std::cout << east;
```

### Output

```text
==================
     | sales  cost
------------------
West | 10     4   
West | 20     8   
------------------
[2 cs x 2 feat]
==================
     | sales  cost
------------------
East | 30     12  
East | 40     16  
------------------
[2 cs x 2 feat]
```

`west` uses the named form (`.label` / `.value`); `east` uses the equivalent `.key`
form, which specifies a value for every case-label level in order instead of naming
each level — here there is only one level (`region`), so `.key = {"East"}` is a
single-element list. Both forms leave `region` in the result's case labels (shown as
the row index) because `drop` was left at its default `false`; pass `drop = true` when
a level's value is now constant across every matched row and no longer worth carrying.

Example 7. Reorder features positionally while keeping a contiguous row block.

```cpp
using Case = DataFrame::Case;
using Feat = DataFrame::Feat;

auto out = orders.case_study(Case::slice(0, 2), Feat::take({3, 1, 4}));
std::cout << out;
```

### Output

```text
====================================
     | units  city   unit_price
------------------------------------
0    | 2      Accra  1200
1    | 5      Accra  45.5
------------------------------------
[2 cs x 3 feat]
```

This is a useful reminder that the feature selector controls output order directly.
`Feat::take({3, 1, 4})` does not sort itself back into source order.

## Important Quirks

- Positional slices and label spans do **not** mean the same thing.
  `Case::slice(1, 4)` is half-open and positional. `Case::span("A100", "D400")` is
  inclusive and label-based.
- `Case::label("B200")` can return several rows. Case labels are lookup keys here, not
  uniqueness guarantees.
- `Case::labels({...})` follows your requested label order, not the frame's original
  order.
- `Case::where(mask)` behaves like `filter(mask)` first, then applies the feature
  selector on the filtered frame.
- `Feat::span("a", "d")` walks the frame's **current feature order**, not
  alphabetical order.

## Failure Modes

These are worth knowing up front because `case_study()` is intentionally strict:

- unknown case label -> validation error
- unknown feature name -> validation error
- out-of-bounds scalar/list position after negative-index normalization -> validation error
- `CasePick.key` length different from the number of case-label levels -> validation error
- `CasePick` with no matches -> validation error
- mixing `CasePick.key` with named level selectors in the same `CasePick` -> validation error
- `CasePick.names` and `CasePick.values` of different lengths -> validation error

One subtle exception: a reversed label span such as `Case::span("D400", "A100")`
does **not** raise. It returns an empty frame because the stop position lands before
the start position.

## Notes

- `Case` and `Feat` should be built via their static factory methods (`Case::at`,
  `Case::slice`, `Case::take`, `Case::label`, `Case::labels`, `Case::span`,
  `Case::where`; `Feat::all`, `Feat::name`, `Feat::at`, `Feat::names`, `Feat::take`,
  `Feat::slice`, `Feat::span`) rather than by setting their fields directly — those
  fields are an implementation detail of how the selector is dispatched internally.
- Positional slices (`Case::slice`, `Feat::slice`) are half-open, exclusive on `stop`
  — the same convention as `slice()`. Label-based ranges (`Case::span`, `Feat::span`)
  are inclusive on both ends — a different convention, worth remembering when
  switching between the two.
- An out-of-range single position (`Case::at(row)` after negative normalization) is a
  validation error, matching `take()`'s behavior, not `slice()`'s silent clamping.
- The `CasePick` overload only works on a frame with a named case axis from
  `label().set(...)`; on a frame using the plain default case label, use
  `Case::label(...)` / `Case::labels(...)` / `Case::span(...)` instead.
- `names` / `values` on `CasePick` generalizes `label` / `value` to matching several
  named levels simultaneously — every listed level must match its paired value for a
  row to be included.
- `Case::span(...)` uses the first occurrence of the start label and the last
  occurrence of the stop label. That is why duplicate labels can expand the result
  more than first-time users expect.
- The selector structs are deliberately small vocabulary objects, not a second query
  language. If your selection logic is already a boolean expression over feature
  values, build the mask and pass it through `Case::where(...)` instead of trying to
  encode it indirectly through labels or positions.

## See Also

- [`DataFrame::take`](take.md): the simpler positional-only primitive `Case::at` /
  `Case::take` selection is built on.
- [`DataFrame::filter`](filter.md): boolean-mask row selection without the feature
  projection `case_study()` also offers.
- [`DataFrame::features`](features.md): a lighter-weight path when you only need
  feature projection and no row selection at all.
- [`DataFrame::label`](label.md): the accessor used to inspect or set the case-label
  axis that `Case::label`, `Case::labels`, `Case::span`, and `CasePick` all read from.
- [`DataFrame::observe`](observe.md): a display-oriented row window, for quick
  previewing rather than a result meant for further computation.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="bucketize.html">
    <span class="label">Previous</span>
    <span class="title">bucketize</span>
  </a>
  <a class="next-link" href="cast.html">
    <span class="label">Next</span>
    <span class="title">cast</span>
  </a>
</div>
