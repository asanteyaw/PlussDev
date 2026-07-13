# DataFrame::get

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::get(std::string_view feature_name) -> std::optional<Thread>
```

## What It Does

Fetches one value, member, or slice from the current object without changing the source object.

## Parameters

**feature_name**
    Name of the target feature to read, rewrite, sort by, or promote to cases.

## Returns

`std::optional<Thread>`
    Value produced by this call.

## Examples

Example 1. Work with one named feature.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.get("sales");
```

### Output

```text
Thread([100.0, 120.0, 90.0])
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="freq.html">
    <span class="label">Previous</span>
    <span class="title">freq</span>
  </a>
  <a class="next-link" href="group_by.html">
    <span class="label">Next</span>
    <span class="title">group_by</span>
  </a>
</div>
