# DataFrame::sort_cases

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::sort_cases(bool ascending = true, bool nulls_last = true) -> DataFrame

DataFrame::sort_cases(const SortCasesOptions& options) -> DataFrame
```

## What It Does

Sorts rows by the current case axis rather than by a data feature.

## Parameters

**ascending**
    When `true`, case labels are sorted ascending. When `false`, they are sorted descending.

**nulls_last**
    When `true`, null case labels are placed after observed labels. When `false`, they are placed first.

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto with_cases = df.with_cases(pluss::table::Thread(pluss::make_note({"cs2", "cs0", "cs1"})));
auto out = with_cases.sort_cases(true, true);
```

### Output

```text
with_cases.caselbl():
["cs2", "cs0", "cs1"]

out.caselbl():
["cs0", "cs1", "cs2"]

out.head():
id  sales  city
2   120.0  Tema
3    90.0  Kumasi
1   100.0  Accra
```
Example 2. Pass an explicit options object to control the operation.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto with_cases = df.with_cases(pluss::table::Thread(pluss::make_note({"cs2", "cs0", "cs1"})));
auto out = with_cases.sort_cases(true, true);
```

### Output

```text
with_cases.caselbl():
["cs2", "cs0", "cs1"]

out.caselbl():
["cs0", "cs1", "cs2"]

out.head():
id  sales  city
2   120.0  Tema
3    90.0  Kumasi
1   100.0  Accra
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="slice.html">
    <span class="label">Previous</span>
    <span class="title">slice</span>
  </a>
  <a class="next-link" href="sort_values.html">
    <span class="label">Next</span>
    <span class="title">sort_values</span>
  </a>
</div>
