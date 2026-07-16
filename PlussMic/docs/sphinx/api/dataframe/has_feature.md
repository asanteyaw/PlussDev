# DataFrame::has_feature

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::has_feature(std::string_view name) -> bool
```

## What It Does

Checks whether a feature name exists in the frame.

## Parameters

**name**
    Name used to identify the target object, field, feature, or slot for this overload.

## Returns

`bool`
    Boolean status or membership result.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.has_feature("sales");
```

### Output

```text
true
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="has.html">
    <span class="label">Previous</span>
    <span class="title">has</span>
  </a>
  <a class="next-link" href="head.html">
    <span class="label">Next</span>
    <span class="title">head</span>
  </a>
</div>
