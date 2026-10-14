<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6d291252f594 -->

# DataFrameLabelAccessor::codes

[Back to DataFrameLabelAccessor API Reference](../../dataframe_label_accessor_api_reference)

## Signature

```cpp
DataFrameLabelAccessor::codes() const -> DataFrame
```

## What It Does

Factorizes the case labels into integer codes, in first-occurrence order (the
first distinct value seen gets code `0`, the next new value gets `1`, and so
on). For a structured (multi-level) frame, each level is factorized
**independently**, producing one integer column per level. For an unstructured
frame, the whole flat case-label slate is factorized into a single `label`
column.

## Returns

`DataFrame`
    One column per case-label level (or a single `label` column if
    unstructured), each holding that level's factorized codes; case labels on
    the result match the source's own case labels.

## Examples

Example 1. Factorize a 2-level structured case axis.

```cpp
using namespace pluss::table;

auto readings = DataFrame(
    {{"region", Slate(pluss::make_note({"east", "east", "west", "west"}))},
     {"city",   Slate(pluss::make_note({"Boston", "NYC", "LA", "SF"}))},
     {"sales",  Slate(pluss::make_series({10.0, 20.0, 30.0, 40.0}))}});
auto grouped = readings.label().set({"region", "city"}, true, "/");
std::cout << grouped;

auto out = grouped.label().codes();
std::cout << out;
```

### Output (before)

```text
===================
            | sales
-------------------
east/Boston | 10   
east/NYC    | 20   
west/LA     | 30   
west/SF     | 40   
-------------------
[4 cs x 1 feat]
```

### Output (after)

```text
==========================
            | region  city
--------------------------
east/Boston | 0       0   
east/NYC    | 0       1   
west/LA     | 1       2   
west/SF     | 1       3   
--------------------------
[4 cs x 2 feat]
```

`region` only ever sees two distinct values (`"east"` first, then `"west"`), so
its codes are `0`/`1`; `city` sees four distinct values in row order, so it
gets a fresh code `0`–`3` for every row.

Example 2. Factorize an unstructured, single flat case axis — one built by
[`set(const Slate&)`](set.md) rather than by promoting a named feature.

```cpp
auto orders = DataFrame(
    {{"sales", Slate(pluss::make_series({100.0, 200.0, 150.0, 90.0}))}});
auto tagged = orders.label().set(Slate(pluss::make_note({"r1", "r2", "r3", "r4"})));

auto out2 = tagged.label().codes();
std::cout << out2;
```

### Output

```text
============
     | label
------------
r1   | 0    
r2   | 1    
r3   | 2    
r4   | 3    
------------
[4 cs x 1 feat]
```

`tagged`'s case labels came from `set(const Slate&)` — the raw-axis overload,
which never populates `case_axis_` — so `.label().has()` is `false` here, and
`codes()` takes the unstructured branch: one column, always named `"label"`,
factorizing the whole flat slate. Contrast with a frame built by promoting a
*named* feature (like `set("region")`), which — even with only one promoted
name — still creates a genuine one-level `case_axis_`, so `codes()` takes the
structured branch instead and names the column after that level (`"region"`,
not `"label"`).

## Notes

- Codes are per-level for structured frames; there is no shared numbering
  across levels.
- The result's own case labels are the source's case labels, unchanged — only
  the feature columns differ.

## See Also

- [`DataFrameLabelAccessor::levels`](levels.md): the corresponding distinct
  values each level's codes index into.
- [`DataFrameLabelAccessor::depth`](depth.md): how many leading levels are
  jointly sorted.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cast.html">
    <span class="label">Previous</span>
    <span class="title">cast</span>
  </a>
  <a class="next-link" href="depth.html">
    <span class="label">Next</span>
    <span class="title">depth</span>
  </a>
</div>
