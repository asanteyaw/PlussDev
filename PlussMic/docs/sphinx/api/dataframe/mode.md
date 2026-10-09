<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 3c9eedb1acd4 -->

# DataFrame::mode

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::mode(bool drop_nulls = true) const -> DataFrame
```

## What It Does

Finds the most frequently occurring value in every compatible feature and returns a
new `DataFrame`, not a `Slate` — like [`percentiles()`](percentiles.md), this
method's result is naturally a small table rather than one value per feature, because
a feature can have **more than one** mode when several values are tied for most
frequent.

Every numeric `Series` feature, plus every `Note`, `Timeline`, and `TimeIndex`
feature, participates — `mode()` is not restricted to numbers the way `sum()` or
`mean()` are, since "most common value" is meaningful for categories and timestamps
too. The result has one row per rank position (`mode_0`, `mode_1`, …) up to however
many-way the widest tie in the frame is; features with fewer tied modes than that are
padded with `NULL` in their remaining rows.

## Parameters

**drop_nulls** (`bool`, default `true`)

Controls whether a missing value can itself become "the mode":

- `true` (the default): missing values never participate. Only observed values are
  candidates, and a feature made entirely of missing values is a validation error
  (there is no value to report).
- `false`: missing values are counted like any other value would be. If missing cells
  outnumber every individual observed value in a feature, that feature's mode becomes
  `NULL` — "the most common thing in this column is that it's empty."

## Returns

`DataFrame`
    A new frame with one row per rank position (labeled `mode_0`, `mode_1`, …) and one
    column per participating feature, holding that feature's mode(s). Features with
    fewer modes than the widest tie in the result are padded with `NULL`.

## Examples

Example 1. Find the single mode of a categorical and a numeric feature.

```cpp
using namespace pluss::table;

auto sales = DataFrame({
    {"region", Slate(pluss::make_note({"West", "East", "West", "North", "East", "West"}))},
    {"units",  Slate(pluss::make_series({2, 5, 2, 3, 5, 2}))},
});

auto common = sales.mode();
std::cout << common;
```

### Output

```text
======================
       | region  units
----------------------
mode_0 | West    2    
----------------------
[1 cs x 2 feat]
```

`region` appears `West` three times (more than `East` twice or `North` once), and
`units` is `2` three times (more than `5` twice or `3` once). Both features have a
single, unambiguous mode, so the result has exactly one row.

Example 2. See a tie produce more than one mode in the same call, and a shorter
column padded with `NULL`.

```cpp
auto sales_tied = DataFrame({
    {"region", Slate(pluss::make_note({"West", "East", "West", "North", "East", "West"}))},
    {"units",  Slate(pluss::make_series({2, 5, 2, 5, 3, 1}))},
});

auto common_tied = sales_tied.mode();
std::cout << common_tied;
```

### Output

```text
======================
       | region  units
----------------------
mode_0 | West    2    
mode_1 | NULL    5    
----------------------
[2 cs x 2 feat]
```

`units` now has a genuine two-way tie: `2` and `5` each occur twice, so both are
reported, in ascending order (`mode_0 = 2`, `mode_1 = 5`). `region` still has a single
mode (`West`, unchanged from Example 1), so its `mode_1` row is padded with `NULL` —
the result's row count is set by the *widest* tie in the frame, not by each feature
individually.

Example 3. Use `drop_nulls = false` to let a missing value itself become the mode.

```cpp
auto status_valid = pluss::make_series({true, false, false, false, true, false}, pluss::ScalarType::Bool);
auto shipments = DataFrame({
    {"status", Slate(pluss::make_note({"shipped", "", "", "", "paid", ""}), status_valid)},
});

auto common_status = shipments.mode();
std::cout << common_status;

auto common_status_with_nulls = shipments.mode(false);
std::cout << common_status_with_nulls;
```

### Output

```text
================
       | status 
----------------
mode_0 | paid   
mode_1 | shipped
----------------
[2 cs x 1 feat]
===============
       | status
---------------
mode_0 | NULL  
---------------
[1 cs x 1 feat]
```

With the default `drop_nulls = true`, only the two observed values are candidates —
`"paid"` and `"shipped"` each occur once, so both are reported as a tie. With
`drop_nulls = false`, the four missing cells outnumber either observed value (`4` vs
`1`), so the mode becomes `NULL` — the most common thing in this column really is that
it's empty.

## Notes

- `mode()`'s result is a `DataFrame`, not a `Slate` — like `percentiles()`, because
  the result can have more than one row per call when ties occur.
- Every feature type participates: numeric `Series`, `Note`, `Timeline`, and
  `TimeIndex` — this is not restricted to numeric features the way `sum()` and
  `mean()` are.
- Ties are reported in ascending sorted order (numerically for `Series`, lexically for
  `Note`), never in original column order or insertion order.
- A feature with no observed values at all (every cell missing, or a numeric feature
  with `drop_nulls = true` and only missing cells) is a validation error.

## See Also

- [`DataFrame::percentiles`](percentiles.md): the other numeric reducer in this family
  that returns a `DataFrame` rather than a `Slate`.
- [`DataFrame::freq`](freq.md): full frequency ranking of every observed value, not
  just the single most frequent one.
- [`DataFrame::ndistinct`](ndistinct.md): counts how many distinct values a feature
  has, without identifying which one is most common.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="mod.html">
    <span class="label">Previous</span>
    <span class="title">mod</span>
  </a>
  <a class="next-link" href="mul.html">
    <span class="label">Next</span>
    <span class="title">mul</span>
  </a>
</div>
