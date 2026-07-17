# DataFrame::count_by

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::count_by(std::string_view feature, std::int64_t top_n = 0, bool dropnull = true, bool render = false) -> DataFrameView
```

Group rows by one feature and return the count for each distinct key.

This is the compact frequency-table surface for one grouping feature.

## Parameters

**feature**

Feature whose distinct values define the groups.

**top_n**

Optional cap on the number of groups returned.

- `0`: keep every group.
- positive integer: keep only the top `N` groups after sorting.

**dropnull**

Missing-key policy.

- `true`: drop missing grouping keys.
- `false`: allow missing keys to participate as their own group.

**render**

Display toggle for the returned view.

## Returns

`DataFrameView`
    Frequency table with the grouping key and its count.

## Notes

Results are sorted by count descending. Ties are then stabilized by key order.

Use `top_n` when you only want the most frequent groups.

## Examples

Example 1. Return counts for every distinct grouping key.

```cpp
using namespace pluss::table;

auto cities = DataFrame({
    {"city", pluss::make_note({"Accra", "Tema", "Accra", "Kumasi", "Accra", "Tema"})},
    {"score", pluss::make_series({10.0, 20.0, 11.0, 30.0, 12.0, 21.0})},
});

auto out = cities.count_by("city", 0, true, false);
```

### Output

```text
====================
     | city    count
--------------------
0    | Accra   3    
1    | Tema    2    
2    | Kumasi  1    
--------------------
[3 cs x 2 feat]
```

Example 2. Keep only the top two most frequent groups.

```cpp
using namespace pluss::table;

auto cities = DataFrame({
    {"city", pluss::make_note({"Accra", "Tema", "Accra", "Kumasi", "Accra", "Tema"})},
    {"score", pluss::make_series({10.0, 20.0, 11.0, 30.0, 12.0, 21.0})},
});

auto out = cities.count_by("city", 2, true, false);
```

### Output

```text
===================
     | city   count
-------------------
0    | Accra  3    
1    | Tema   2    
-------------------
[2 cs x 2 feat]
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="count.html">
    <span class="label">Previous</span>
    <span class="title">count</span>
  </a>
  <a class="next-link" href="covar.html">
    <span class="label">Next</span>
    <span class="title">covar</span>
  </a>
</div>
