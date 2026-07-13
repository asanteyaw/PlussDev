# DataFrame::note

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::note(pluss::RangeView<std::string_view> features = {}) -> DataFrameNoteAccessor

DataFrame::note(std::string_view feature) -> DataFrameNoteAccessor
```

## What It Does

Returns one named feature as a note-backed thread.

## Parameters

**features**
    Feature name(s) that select the target DataFrame columns for the operation.

**feature**
    Name of the source feature used by the operation.

## Returns

`DataFrameNoteAccessor`
    Value produced by this call.

## Examples

Example 1. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.note();
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 2. Work with one named feature.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.note();
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nfeatures.html">
    <span class="label">Previous</span>
    <span class="title">nfeatures</span>
  </a>
  <a class="next-link" href="note_features.html">
    <span class="label">Next</span>
    <span class="title">note_features</span>
  </a>
</div>
