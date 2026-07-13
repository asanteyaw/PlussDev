# DataFrame::tl

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::tl(const std::vector<std::string>& features) -> DataFrameTimelineAccessor

DataFrame::tl(pluss::RangeView<std::string_view> features = {}) -> DataFrameTimelineAccessor

DataFrame::tl(std::initializer_list<std::string_view> features) -> DataFrameTimelineAccessor

DataFrame::tl(std::string_view feature) -> DataFrameTimelineAccessor
```

## What It Does

This is part of the time-aware surface and expects timeline or time-index semantics.

## Parameters

**features**
    Feature name(s) that select the target DataFrame columns for the operation.

**feature**
    Name of the source feature used by the operation.

## Returns

`DataFrameTimelineAccessor`
    Value produced by this call.

## Examples

Example 1. Pass several names together from a standard string container.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.tl();
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 2. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.tl();
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 3. Pass several feature names inline.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.tl();
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 4. Work with one named feature.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.tl();
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="title_.html">
    <span class="label">Previous</span>
    <span class="title">title_</span>
  </a>
  <a class="next-link" href="to_string.html">
    <span class="label">Next</span>
    <span class="title">to_string</span>
  </a>
</div>
