# DataFrame::tally

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::tally(pluss::RangeView<std::string_view> features, std::int64_t top_n = 0, bool dropnull = true, bool render = false) -> DataFrameView

DataFrame::tally(std::int64_t top_n = 0, bool dropnull = true, bool render = false) -> DataFrameView

DataFrame::tally(std::string_view feature, std::int64_t top_n = 0, bool dropnull = true, bool render = false) -> DataFrameView
```

## What It Does

Counts how many rows fall into each distinct value of the grouping feature.

## Parameters

**features**
    Feature name(s) that select the target DataFrame columns for the operation.

**top_n**
    Optional cap on how many grouped rows to keep after sorting. Use `0` to keep every group.

**dropnull**
    When `true`, null grouping keys or null values are excluded where the method supports that policy. When `false`, null participates instead of being removed.

**render**
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

**feature**
    Name of the source feature used by the operation.

## Returns

`DataFrameView`
    Value produced by this call.

## Examples

Example 1. Count distinct combinations across several features.

```cpp
auto df = pluss::table::DataFrame({
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Accra", "Tema", "Tema"}))},
    {"product", pluss::table::Thread(pluss::make_note({"A", "B", "A", "B"}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0, 150.0}))},
});

auto out = df.tally({"city", "product"}, 5, true, false);
```

### Output

```text
city    count
Accra   2
Tema    2
```
Example 2. Count distinct row groups using the default tally target.

```cpp
auto df = pluss::table::DataFrame({
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Accra", "Tema", "Tema"}))},
    {"product", pluss::table::Thread(pluss::make_note({"A", "B", "A", "B"}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0, 150.0}))},
});

auto out = df.tally(3, true, false);
```

### Output

```text
city    count
Accra   2
Tema    2
```
Example 3. Count the distinct values in one named feature.

```cpp
auto df = pluss::table::DataFrame({
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Accra", "Tema", "Tema"}))},
    {"product", pluss::table::Thread(pluss::make_note({"A", "B", "A", "B"}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0, 150.0}))},
});

auto out = df.tally("city", 3, true, false);
```

### Output

```text
city    count
Accra   2
Tema    2
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="take.html">
    <span class="label">Previous</span>
    <span class="title">take</span>
  </a>
  <a class="next-link" href="title.html">
    <span class="label">Next</span>
    <span class="title">title</span>
  </a>
</div>
