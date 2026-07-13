# DataFrame::rename_

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::rename_(std::string_view old_name, std::string new_name) -> DataFrame&
```

## What It Does

Renames one feature in place without changing its values.

## Parameters

**old_name**
    Existing feature name to rename in place.

**new_name**
    Replacement feature name written back in place.

## Returns

`DataFrame&`
    Value produced by this call.

## Examples

Example 1. Rename one column in place by giving its old name and its new name.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.rename_("sales", "revenue");
```

### Output

```text
id  revenue  city
1   100.0    Accra
2   120.0    Tema
3    90.0    Kumasi
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rename.html">
    <span class="label">Previous</span>
    <span class="title">rename</span>
  </a>
  <a class="next-link" href="rename_levels.html">
    <span class="label">Next</span>
    <span class="title">rename_levels</span>
  </a>
</div>
