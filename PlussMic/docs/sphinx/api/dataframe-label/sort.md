<!-- pluss-hand-authored -->

# DataFrameLabelAccessor::sort

[Back to DataFrameLabelAccessor API Reference](../../dataframe_label_accessor_api_reference)

## Signatures

```cpp
DataFrameLabelAccessor::sort(bool ascending = true, bool nulls_last = true) const -> DataFrame
DataFrameLabelAccessor::sort(const DataFrame::SortLabelOptions& options) const -> DataFrame
```

## What It Does

Reorders every row by case label. The simple overload compares the **flat**
case label as a single sequence (the same comparison
[`monotonic()`](monotonic.md) uses) — on a structured, multi-level frame, this
sorts by the composite *displayed* string, not level-by-level. `sort(options)`
with `options.names` given instead sorts **lexicographically by those named
levels**, in the order you list them — a real per-level sort, independent of
how the composite string happens to be joined.

## Parameters

**ascending** (`bool`, default `true`)

Sort ascending if `true`, descending if `false`.

**nulls_last** (`bool`, default `true`)

Where null case labels sort relative to real values.

**options** (`const DataFrame::SortLabelOptions&`)

`names` (level names to sort by, in priority order — leave empty to fall back
to the flat comparison), plus `ascending`/`nulls_last`.

## Returns

`DataFrame`
    A new frame with the same rows, reordered by case label.

## Examples

Example 1. Sort a single-level structured frame ascending and descending.

```cpp
using namespace pluss::table;

auto orders = DataFrame(
    {{"region", Thread(pluss::make_note({"east", "west", "east", "north"}))},
     {"sales",  Thread(pluss::make_series({100.0, 200.0, 150.0, 90.0}))}});
auto labeled = orders.label().set("region", false);
std::cout << labeled;

auto ascending = labeled.label().sort();
std::cout << ascending;
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

### Output (ascending)

```text
=====================
      | region  sales
---------------------
east  | east    100  
east  | east    150  
north | north   90   
west  | west    200  
---------------------
[4 cs x 2 feat]
```

```cpp
auto descending = labeled.label().sort(false);
std::cout << descending;
```

### Output (descending)

```text
=====================
      | region  sales
---------------------
west  | west    200  
north | north   90   
east  | east    100  
east  | east    150  
---------------------
[4 cs x 2 feat]
```

Ties (the two `"east"` rows) keep their original relative order either way —
the sort is stable.

Example 2. Sort a 2-level structured frame by one named level ahead of the
other, instead of the composite string's own left-to-right order.

```cpp
using namespace pluss::table;

auto readings = DataFrame(
    {{"region", Thread(pluss::make_note({"west", "east", "west", "east"}))},
     {"city",   Thread(pluss::make_note({"LA", "Boston", "SF", "NYC"}))},
     {"sales",  Thread(pluss::make_series({30.0, 10.0, 40.0, 20.0}))}});
auto grouped = readings.label().set({"region", "city"}, true, "/");
std::cout << grouped;

auto flat_sorted = grouped.label().sort();
std::cout << flat_sorted;

auto city_first = grouped.label().sort(DataFrame::SortLabelOptions{.names = {"city", "region"}});
std::cout << city_first;
```

### Output (grouped)

```text
===================
            | sales
-------------------
west/LA     | 30   
east/Boston | 10   
west/SF     | 40   
east/NYC    | 20   
-------------------
[4 cs x 1 feat]
```

### Output (flat_sorted — composite string order)

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

### Output (city_first — per-level order, city ahead of region)

```text
===================
            | sales
-------------------
east/Boston | 10   
west/LA     | 30   
east/NYC    | 20   
west/SF     | 40   
-------------------
[4 cs x 1 feat]
```

The composite label is `"region/city"`, so the plain `sort()` above sorts by
`region` first (`east` before `west`) and `city` only breaks ties within it.
`sort(SortLabelOptions{.names = {"city", "region"}})` instead sorts by `city`
first regardless of level order in the composite string — `"Boston" <
"LA" < "NYC" < "SF"` puts `west/LA` right after `east/Boston`, ahead of
`east/NYC`, which the flat composite sort would never produce.

## Notes

- The simple overload compares the flat, displayed case-label string on a
  structured frame — if the composite join could ever be ambiguous (rare with
  the default separator, more of a concern with a custom one), prefer
  `sort(SortLabelOptions{.names = {...}})` for an unambiguous, per-level sort.
- Sorting is stable: rows with equal case labels retain their original
  relative order.

## See Also

- [`DataFrameLabelAccessor::monotonic`](monotonic.md): check sortedness without
  reordering.
- [`DataFrameLabelAccessor::depth`](depth.md): how many leading levels are
  already jointly sorted.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="slice.html">
    <span class="label">Previous</span>
    <span class="title">slice</span>
  </a>
  <a class="next-link" href="swap.html">
    <span class="label">Next</span>
    <span class="title">swap</span>
  </a>
</div>
