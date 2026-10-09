<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 8367845ae0f8 -->

# DataFrame::unstack

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::unstack(const UnstackOptions& options) const -> DataFrame
DataFrame::unstack(std::string_view label) const  // shorthand for UnstackOptions{.label = label}
```

## What It Does

Pivots one named level out of a **multi-level case-label axis** so its distinct
values become new output columns — the (partial) inverse of
[`stack()`](stack.md)/[`pivot_table()`](pivot_table.md). `unstack()` requires the
frame to already have named case-label levels (built via
[`label().set({...}, true)`](label.md)); the level named by `options.label` (or the
convenience overload's `label` argument) is the one that gets spread into columns,
while every *other* level stays as the row grouping key.

Whether the new column names get prefixed by the source feature name depends on how
many ordinary features remain on the frame at the time of the call: with exactly one
remaining feature, the pivoted columns are named directly after the level's distinct
values; with more than one, each gets prefixed as `feature + options.separator +
value` to keep them distinguishable.

## Parameters

**options** (`const UnstackOptions&`)

- **`label`** (`std::string`): the case-label level to pivot into columns. Must
  already exist on the frame's case-label axis.
- **`drop`** (`bool`, default `true`): whether the *remaining* case-label level(s)
  end up promoted back into the result's case-label axis (`true`) or kept as
  ordinary columns instead (`false`).
- **`sort`** (`bool`, default `true`): whether the pivoted column names are sorted
  alphabetically before being added.
- **`separator`** (`std::string`, default `"_"`): the separator between a source
  feature name and a pivoted value, used only when more than one feature remains
  (see above).

**label** (`std::string_view`, convenience overload)

Shorthand for `UnstackOptions{.label = label}` — every other option keeps its
default (`drop = true`, `sort = true`, `separator = "_"`).

## Returns

`DataFrame`
    A new frame with one row per distinct combination of the *remaining* case-label
    level(s), and one column per distinct value of the pivoted `label`.

## Examples

Example 1. Pivot one level out of a two-level case-label axis, with a single
remaining feature (no column prefix).

```cpp
using namespace pluss::table;

auto grades = DataFrame({
    {"student", Slate(pluss::make_note({"Ada", "Ada", "Kojo", "Kojo"}))},
    {"term",    Slate(pluss::make_note({"mid", "final", "mid", "final"}))},
    {"score",   Slate(pluss::make_series({88.0, 91.0, 84.0, 89.0}))},
});
std::cout << grades;
```

### Output (before)

```text
============================
     | student  term   score
----------------------------
0    | Ada      mid    88   
1    | Ada      final  91   
2    | Kojo     mid    84   
3    | Kojo     final  89   
----------------------------
[4 cs x 3 feat]
```

```cpp
auto keyed = grades.label().set({"student", "term"}, /*drop_features=*/true, "|");
auto by_term = keyed.unstack("term");
std::cout << by_term;
```

### Output (after)

```text
==================
     | final  mid 
------------------
Ada  | 91     88  
Kojo | 89     84  
------------------
[2 cs x 2 feat]
```

`"student"` and `"term"` both became case-label levels (dropped as ordinary
features), leaving `"score"` as the only remaining feature. `unstack("term")` pivots
`term`'s two distinct values (`"final"`, `"mid"`, alphabetically sorted since
`sort = true` by default) into columns, and — since only one feature (`score`)
remained — those columns are named directly `final`/`mid`, with no `score_` prefix.
`"student"`, the one remaining level, is promoted back into the result's case
labels (`drop = true` by default), rather than kept as a visible column.

Example 2. With more than one remaining feature, pivoted columns get prefixed by
feature name.

```cpp
auto grades_with_attendance = DataFrame({
    {"student",    Slate(pluss::make_note({"Ada", "Ada", "Kojo", "Kojo"}))},
    {"term",       Slate(pluss::make_note({"mid", "final", "mid", "final"}))},
    {"score",      Slate(pluss::make_series({88.0, 91.0, 84.0, 89.0}))},
    {"attendance", Slate(pluss::make_series({95.0, 90.0, 98.0, 92.0}))},
});

auto keyed_multi = grades_with_attendance.label().set({"student", "term"}, true, "|");
auto wide = keyed_multi.unstack("term");
std::cout << wide;
```

### Output

```text
===============================================================
     | score_final  score_mid  attendance_final  attendance_mid
---------------------------------------------------------------
Ada  | 91           88         90                95            
Kojo | 89           84         92                98            
---------------------------------------------------------------
[2 cs x 4 feat]
```

Now that **two** features (`score` and `attendance`) remain after promoting
`student`/`term`, every pivoted column is prefixed with its source feature name and
`options.separator` (`"_"` by default) — `score_final`, `score_mid`,
`attendance_final`, `attendance_mid`.

## Notes

- `unstack()` requires the frame to already have named case-label level(s) — build
  them first with [`label().set({...}, true)`](label.md); it throws
  `"DataFrame::unstack: current DataFrame has no case labels"` otherwise.
- If a `(remaining-level combination, pivoted value)` pair occurs more than once in
  the source, `unstack()` throws `"duplicate cells found; aggregate before
  unstacking"` rather than silently picking one — aggregate with
  [`pivot_table()`](pivot_table.md) first if that's the actual intent.
- A remaining-level combination that never occurs for a given pivoted value produces
  a `NULL` cell there, the same convention [`pivot_table()`](pivot_table.md) uses.

## See Also

- [`DataFrame::stack`](stack.md): the general melt this method partially reverses.
- [`DataFrame::pivot_table`](pivot_table.md): the aggregating wide-reshape for
  ordinary (non-case-label) features, a related but distinct reshape.
- [`DataFrame::label`](label.md): building and inspecting the case-label levels
  `unstack()` operates on.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="unsqueeze.html">
    <span class="label">Previous</span>
    <span class="title">unsqueeze</span>
  </a>
  <a class="next-link" href="update.html">
    <span class="label">Next</span>
    <span class="title">update</span>
  </a>
</div>
