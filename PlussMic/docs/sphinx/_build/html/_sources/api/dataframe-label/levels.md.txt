<!-- pluss-hand-authored -->

# DataFrameLabelAccessor::levels

[Back to DataFrameLabelAccessor API Reference](../../dataframe_label_accessor_api_reference)

## Signature

```cpp
DataFrameLabelAccessor::levels() const -> DataFrame
```

## What It Does

Lists the distinct values of each case-label level, one column per level, in
first-occurrence order — the values that [`codes()`](codes.md)'s integer
codes index into. For an unstructured frame, this is a single `label` column of
the flat case-label thread's own distinct values.

Levels naturally hold different numbers of distinct values; shorter columns
are padded to match the longest one.

## Returns

`DataFrame`
    One column per level (or a single `label` column if unstructured), holding
    that level's distinct values, default integer case labels.

## Examples

Example 1. Distinct values per level, with `region` shorter than `city`.

```cpp
using namespace pluss::table;

auto readings = DataFrame(
    {{"region", Thread(pluss::make_note({"east", "east", "west", "west"}))},
     {"city",   Thread(pluss::make_note({"Boston", "NYC", "LA", "SF"}))},
     {"sales",  Thread(pluss::make_series({10.0, 20.0, 30.0, 40.0}))}});
auto grouped = readings.label().set({"region", "city"}, true, "/");

auto out = grouped.label().levels();
std::cout << out;
```

### Output

```text
=====================
     | region  city  
---------------------
0    | east    Boston
1    | west    NYC   
2    |         LA    
3    |         SF    
---------------------
[4 cs x 2 feat]
```

`region` only ever has two distinct values (`"east"`, `"west"`); `city` has
four. `region`'s column is padded with **blank cells** to match `city`'s
length — not the project's usual `"NULL"` marker, since these blanks aren't
missing data, just padding to align columns of different lengths.

## Notes

- The padding cells are a genuine empty string, not `"NULL"` — don't confuse
  a padded blank with an actual null case-label value. A real null *within* a
  level's own distinct values (if `dropnull` behavior ever let one through)
  would still print as `"NULL"`; only the trailing padding is blank.
- Case labels on the result are always the default integer index, regardless
  of the source frame's own case labels.

## See Also

- [`DataFrameLabelAccessor::codes`](codes.md): the integer codes that index
  into these same distinct values.
- [`DataFrameLabelAccessor::names`](names.md): the level names these columns
  are titled after.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="isnull.html">
    <span class="label">Previous</span>
    <span class="title">isnull</span>
  </a>
  <a class="next-link" href="map.html">
    <span class="label">Next</span>
    <span class="title">map</span>
  </a>
</div>
