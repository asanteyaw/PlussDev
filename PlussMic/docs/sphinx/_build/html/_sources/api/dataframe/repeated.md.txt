<!-- pluss-hand-authored -->

# DataFrame::repeated

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::repeated(pluss::RangeView<std::string_view> features = {}) const -> Thread
```

## What It Does

Flags **every** row that belongs to a group sharing the same composite key across
`features` — including that group's very first occurrence, not just the ones after
it. This is exactly [`duplicated(features, false, false)`](duplicated.md) with its
display name changed to `"repeated"`: `repeated()` is what you reach for when you
want "is this row part of a repeated group at all," rather than "is this row a
duplicate follower of an earlier one" (which is what `duplicated()`'s default
`keep_first = true` answers instead).

## Parameters

**features** (`pluss::RangeView<std::string_view>`, default `{}`)

Which features form the composite key that defines a "group." Leave empty (the
default) to compare whole rows across every feature.

## Returns

`Thread`
    A boolean `Series`, one entry per case, labeled `"repeated"`. `true` marks every
    row belonging to a key that occurs more than once anywhere in the frame; `false`
    marks a row whose key is unique.

## Examples

Example 1. Flag every row belonging to a repeated student group.

```cpp
using namespace pluss::table;

auto roster = DataFrame({
    {"student", Thread(pluss::make_note({"Ada", "Bola", "Ada", "Chen", "Bola"}))},
});
std::cout << roster;
```

### Output

```text
==============
     | student
--------------
0    | Ada    
1    | Bola   
2    | Ada    
3    | Chen   
4    | Bola   
--------------
[5 cs x 1 feat]
```

```cpp
auto groups = roster.repeated({"student"});
std::cout << groups;
```

### Output

```text
0  true
1  true
2  true
3  false
4  true
Name: repeated, ptype: Series
```

`"Ada"` and `"Bola"` each occur twice, so every row for either name is `true`,
including case `0` and case `1` — their **first** appearances. `"Chen"` occurs only
once, so case `3` is `false`.

Example 2. Compare against `duplicated()`'s default follower-only semantics on the
same data.

```cpp
auto followers = roster.duplicated({"student"});
std::cout << followers;
```

### Output

```text
0  false
1  false
2  true
3  false
4  true
Name: duplicated, ptype: Series
```

`duplicated()`'s default (`keep_first = true`) marks only the **second and later**
occurrence of each repeated name (`false` at cases `0` and `1`, the first
appearances) — `repeated()` marks **all** of them, first appearance included. The two
methods answer genuinely different questions: "which rows are extras I could drop"
(`duplicated()`) versus "which rows belong to a group that repeats at all"
(`repeated()`).

## Notes

- `repeated()` is literally `duplicated(features, /*keep_first=*/false,
  /*keep_last=*/false)` under a different display name — it never has its own
  separate keep-policy parameters.
- A missing value in every selected feature is itself a valid "value" for grouping
  purposes — two rows that are both missing on every selected feature count as
  sharing a key, same as [`duplicated()`](duplicated.md).
- Every selected feature name must exist on the frame; an unknown name throws.

## See Also

- [`DataFrame::duplicated`](duplicated.md): the keep-policy-driven sibling, for
  marking only the extras after (or before) the first occurrence.
- [`DataFrame::drop_duplicates`](drop_duplicates.md): removes duplicate rows outright
  instead of just flagging them.
- [`DataFrame::freq`](freq.md): counts *how many times* each composite key occurs,
  rather than flagging individual rows.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="reorder.html">
    <span class="label">Previous</span>
    <span class="title">reorder</span>
  </a>
  <a class="next-link" href="replace.html">
    <span class="label">Next</span>
    <span class="title">replace</span>
  </a>
</div>
