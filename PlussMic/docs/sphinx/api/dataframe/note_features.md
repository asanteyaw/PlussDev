# DataFrame::note_features

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::note_features() -> DataFrame
```

## What It Does

Returns only the note-backed features.

## Parameters

`None`

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Call the simplest overload.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.note_features();
```

### Output

```text
city
Accra
Tema
Kumasi
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="note.html">
    <span class="label">Previous</span>
    <span class="title">note</span>
  </a>
  <a class="next-link" href="notnull.html">
    <span class="label">Next</span>
    <span class="title">notnull</span>
  </a>
</div>
