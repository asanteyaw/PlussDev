<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 566b0cabd8df -->

# DataFrameLabelAccessor::align

[Back to DataFrameLabelAccessor API Reference](../../dataframe_label_accessor_api_reference)

## Signatures

```cpp
DataFrameLabelAccessor::align(const Slate& new_label) const -> DataFrame
DataFrameLabelAccessor::align(const Slate& new_label, std::optional<double> fill) const -> DataFrame
```

## What It Does

Reindexes every feature onto an entirely new set of case labels, matching each
target label to a row of the current case labels **by value**, not by position.
When a target label appears more than once, its occurrences are matched in
order against the source's occurrences of that same label (first target
occurrence to first source occurrence, second to second, and so on); a target
occurrence beyond the number of matching source rows — or a label absent from
the source entirely — comes back with no match.

Unmatched cells are `NULL` for every feature. If `fill` is supplied, unmatched
cells in **numeric (`Series`)** features are replaced by that constant instead
— non-numeric features (`Note`, `Timeline`, `TimeIndex`) stay `NULL` regardless
of `fill`, since there's no numeric value to substitute.

Equivalent to `frame.align(DataFrame::AlignSpec{.label = new_label[, .fill =
fill]})` with every feature selected.

## Parameters

**new_label** (`const Slate&`)

The new case labels to reindex onto. Becomes the result's case labels exactly,
including any repeats.

**fill** (`std::optional<double>`, one overload)

A constant substituted into unmatched cells of numeric features only. Leave
`std::nullopt` (the other overload) to leave unmatched cells `NULL` everywhere.

## Returns

`DataFrame`
    A new frame with `new_label` as its case labels, and every feature
    reindexed by label-value match.

## Examples

Example 1. Reindex onto a label set with a repeat and one label absent from the
source.

```cpp
using namespace pluss::table;

auto orders = DataFrame(
    {{"region", Slate(pluss::make_note({"east", "west", "east", "north"}))},
     {"sales",  Slate(pluss::make_series({100.0, 200.0, 150.0, 90.0}))}});
auto labeled = orders.label().set("region", false);
std::cout << labeled;

auto target = Slate(pluss::make_note({"west", "east", "south"}));
auto out = labeled.label().align(target);
std::cout << out;
```

### Output (before)

```text
=====================
      | region  sales
---------------------
east  | east    100  
west  | west    200  
east  | east    150  
north | north   90   
---------------------
[4 cs x 2 feat]
```

### Output (no fill)

```text
=====================
      | region  sales
---------------------
west  | west    200  
east  | east    100  
south | NULL    NULL 
---------------------
[3 cs x 2 feat]
```

`"west"` and `"east"` each match their first (only, in `"west"`'s case; first of
two, in `"east"`'s) occurrence in the source; `"south"` never appears in
`labeled`'s case labels at all, so **every** feature is `NULL` for that row —
including `region`, even though it's a `Note` column.

Example 2. The same reindex, with a fill value — which only reaches the numeric
feature.

```cpp
auto out2 = labeled.label().align(target, 0.0);
std::cout << out2;
```

### Output

```text
=====================
      | region  sales
---------------------
west  | west    200  
east  | east    100  
south | NULL    0    
---------------------
[3 cs x 2 feat]
```

`sales`'s unmatched cell becomes `0` (the supplied fill), but `region`'s stays
`NULL` — `fill` is a `double` and only ever substitutes into numeric features.

Example 3. A target with more repeats of a label than the source has —
matched positionally, then exhausted.

```cpp
auto target2 = Slate(pluss::make_note({"east", "east", "east"}));
auto out3 = labeled.label().align(target2);
std::cout << out3;
```

### Output

```text
====================
     | region  sales
--------------------
east | east    100  
east | east    150  
east | NULL    NULL 
--------------------
[3 cs x 2 feat]
```

The first two `"east"` targets match the source's two `"east"` rows in order
(`100`, then `150`); the third has no third `"east"` row to draw from, so it's
unmatched.

## Notes

- Matching is purely by label **value**, occurrence-by-occurrence — not by the
  source's row position or any sort order.
- `fill` never reaches non-numeric features; an unmatched `Note`/`Timeline`/
  `TimeIndex` cell is always `NULL`.
- This is the DataFrame-wide counterpart to
  [`DataFrameTsAccessor::align_to`](../dataframe-ts/align_to.md), which performs
  the analogous reindex for time axes using tick-based matching modes instead of
  plain value equality.

## See Also

- [`DataFrame::align`](../dataframe/align.md): the general form this method
  wraps, which also lets you restrict the reindex to a subset of features.
- [`DataFrameLabelAccessor::set`](set.md): promotes a feature to case labels in
  the first place, as used to build `labeled` above.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="cast.html">
    <span class="label">Next</span>
    <span class="title">cast</span>
  </a>
</div>
