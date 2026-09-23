<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: fe6b9c7e905c -->

# DataFrameLabelAccessor::sort

[Back to DataFrameLabelAccessor API Reference](../../dataframe_label_accessor_api_reference)

## Signatures

```cpp
DataFrameLabelAccessor::sort(bool ascending = true, bool nulls_last = true) const -> DataFrame
DataFrameLabelAccessor::sort(const DataFrame::SortLabelOptions& options) const -> DataFrame
```

## What It Does

Reorders every row by case label. On a structured, multi-level case axis, the
simple overload sorts **lexicographically by the stored case-label levels in
their existing order**: first level primary, second level tie-breaker, third
level next tie-breaker, and so on. `sort(options)` with `options.names` given
does the same kind of lexicographic comparison, but using the named levels in
the order you provide rather than the frame's current case-axis order.

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
other, instead of the case axis's existing level order.

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

### Output (flat_sorted — existing level order: region, then city)

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

The case axis stores `region` first and `city` second, so the plain `sort()`
above sorts by `region` first (`east` before `west`) and `city` only breaks
ties within it.
`sort(SortLabelOptions{.names = {"city", "region"}})` instead sorts by `city`
first regardless of the case axis's existing level order — `"Boston" <
"LA" < "NYC" < "SF"` puts `west/LA` right after `east/Boston`, ahead of
`east/NYC`, which the default region-first label sort would never produce.

## Notes

- The simple overload already uses the full multi-level case axis lexicographically;
  it is not limited to a naïve string compare of the rendered joined label.
- Use `sort(SortLabelOptions{.names = {...}})` when you need a different level
  priority from the frame's existing case-axis order.
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
